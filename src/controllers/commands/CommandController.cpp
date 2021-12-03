#include "CommandController.hpp"

#include "Application.hpp"
#include "boost/filesystem.hpp"
#include "common/SignalVector.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/CombinePath.hpp"
#include "util/FormatTime.hpp"
#include "util/Helpers.hpp"
#include "util/IncognitoBrowser.hpp"
#include "util/StreamLink.hpp"
#include "util/Twitch.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/splits/Split.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QUrl>

namespace {
using namespace chatterino;

static const QStringList twitchDefaultCommands{
    "/help",
    "/w",
    "/me",
    "/disconnect",
    "/mods",
    "/vips",
    "/color",
    "/commercial",
    "/mod",
    "/unmod",
    "/vip",
    "/unvip",
    "/ban",
    "/unban",
    "/timeout",
    "/untimeout",
    "/slow",
    "/slowoff",
    "/r9kbeta",
    "/r9kbetaoff",
    "/emoteonly",
    "/emoteonlyoff",
    "/clear",
    "/subscribers",
    "/subscribersoff",
    "/followers",
    "/followersoff",
    "/host",
    "/unhost",
    "/raid",
    "/unraid",
};

static const QStringList whisperCommands{"/w", ".w"};

// stripUserName removes any @ prefix or , suffix to make it more suitable for command use
void stripUserName(QString &userName)
{
    if (userName.startsWith('@'))
    {
        userName.remove(0, 1);
    }
    if (userName.endsWith(','))
    {
        userName.chop(1);
    }
}

// stripChannelName removes any @ prefix or , suffix to make it more suitable for command use
void stripChannelName(QString &channelName)
{
    if (channelName.startsWith('@') || channelName.startsWith('#'))
    {
        channelName.remove(0, 1);
    }
    if (channelName.endsWith(','))
    {
        channelName.chop(1);
    }
}

void sendWhisperMessage(const QString &text)
{
    // (hemirt) pajlada: "we should not be sending whispers through jtv, but
    // rather to your own username"
    auto app = getApp();
    app->twitch.server->sendMessage("jtv", text.simplified());
}

bool appendWhisperMessageWordsLocally(const QStringList &words)
{
    auto app = getApp();

    MessageBuilder b;

    b.emplace<TimestampElement>();
    b.emplace<TextElement>(app->accounts->twitch.getCurrent()->getUserName(),
                           MessageElementFlag::Text, MessageColor::Text,
                           FontStyle::ChatMediumBold);
    b.emplace<TextElement>("->", MessageElementFlag::Text,
                           getApp()->themes->messages.textColors.system);
    b.emplace<TextElement>(words[1] + ":", MessageElementFlag::Text,
                           MessageColor::Text, FontStyle::ChatMediumBold);

    const auto &acc = app->accounts->twitch.getCurrent();
    const auto &accemotes = *acc->accessEmotes();
    const auto &bttvemotes = app->twitch.server->getBttvEmotes();
    const auto &ffzemotes = app->twitch.server->getFfzEmotes();
    auto flags = MessageElementFlags();
    auto emote = boost::optional<EmotePtr>{};
    for (int i = 2; i < words.length(); i++)
    {
        {  // Twitch emote
            auto it = accemotes.emotes.find({words[i]});
            if (it != accemotes.emotes.end())
            {
                b.emplace<EmoteElement>(it->second,
                                        MessageElementFlag::TwitchEmote);
                continue;
            }
        }  // Twitch emote

        {  // bttv/ffz emote
            if ((emote = bttvemotes.emote({words[i]})))
            {
                flags = MessageElementFlag::BttvEmote;
            }
            else if ((emote = ffzemotes.emote({words[i]})))
            {
                flags = MessageElementFlag::FfzEmote;
            }
            if (emote)
            {
                b.emplace<EmoteElement>(emote.get(), flags);
                continue;
            }
        }  // bttv/ffz emote
        {  // emoji/text
            for (auto &variant : app->emotes->emojis.parse(words[i]))
            {
                constexpr const static struct {
                    void operator()(EmotePtr emote, MessageBuilder &b) const
                    {
                        b.emplace<EmoteElement>(emote,
                                                MessageElementFlag::EmojiAll);
                    }
                    void operator()(const QString &string,
                                    MessageBuilder &b) const
                    {
                        auto linkString = b.matchLink(string);
                        if (linkString.isEmpty())
                        {
                            b.emplace<TextElement>(string,
                                                   MessageElementFlag::Text);
                        }
                        else
                        {
                            b.addLink(string, linkString);
                        }
                    }
                } visitor;
                boost::apply_visitor(
                    [&b](auto &&arg) {
                        visitor(arg, b);
                    },
                    variant);
            }  // emoji/text
        }
    }

    b->flags.set(MessageFlag::DoNotTriggerNotification);
    b->flags.set(MessageFlag::Whisper);
    auto messagexD = b.release();

    app->twitch.server->whispersChannel->addMessage(messagexD);

    auto overrideFlags = boost::optional<MessageFlags>(messagexD->flags);
    overrideFlags->set(MessageFlag::DoNotLog);

    if (getSettings()->inlineWhispers)
    {
        app->twitch.server->forEachChannel(
            [&messagexD, overrideFlags](ChannelPtr _channel) {
                _channel->addMessage(messagexD, overrideFlags);
            });
    }

    return true;
}

bool appendWhisperMessageStringLocally(const QString &textNoEmoji)
{
    QString text = getApp()->emotes->emojis.replaceShortCodes(textNoEmoji);
    QStringList words = text.split(' ', QString::SkipEmptyParts);

    if (words.length() == 0)
    {
        return false;
    }

    QString commandName = words[0];

    if (whisperCommands.contains(commandName, Qt::CaseInsensitive))
    {
        if (words.length() > 2)
        {
            return appendWhisperMessageWordsLocally(words);
        }
    }
    return false;
}

const std::map<QString,
               std::function<QString(const QString &, const ChannelPtr &)>>
    COMMAND_VARS{
        {
            "channel.name",
            [](const auto &altText, const auto &channel) {
                (void)(altText);  //unused
                return channel->getName();
            },
        },
        {
            "channel.id",
            [](const auto &altText, const auto &channel) {
                auto *tc = dynamic_cast<TwitchChannel *>(channel.get());
                if (tc == nullptr)
                {
                    return altText;
                }

                return tc->roomId();
            },
        },
        {
            "stream.game",
            [](const auto &altText, const auto &channel) {
                auto *tc = dynamic_cast<TwitchChannel *>(channel.get());
                if (tc == nullptr)
                {
                    return altText;
                }
                const auto &status = tc->accessStreamStatus();
                return status->live ? status->game : altText;
            },
        },
        {
            "stream.title",
            [](const auto &altText, const auto &channel) {
                auto *tc = dynamic_cast<TwitchChannel *>(channel.get());
                if (tc == nullptr)
                {
                    return altText;
                }
                const auto &status = tc->accessStreamStatus();
                return status->live ? status->title : altText;
            },
        },
        {
            "my.id",
            [](const auto &altText, const auto &channel) {
                (void)(channel);  //unused
                auto uid = getApp()->accounts->twitch.getCurrent()->getUserId();
                return uid.isEmpty() ? altText : uid;
            },
        },
        {
            "my.name",
            [](const auto &altText, const auto &channel) {
                (void)(channel);  //unused
                auto name =
                    getApp()->accounts->twitch.getCurrent()->getUserName();
                return name.isEmpty() ? altText : name;
            },
        },
    };

}  // namespace

namespace chatterino {

void CommandController::initialize(Settings &, Paths &paths)
{
    this->commandAutoCompletions_ = twitchDefaultCommands;

    // Update commands map when the vector of commands has been updated
    auto addFirstMatchToMap = [this](auto args) {
        this->userCommands_.remove(args.item.name);

        for (const Command &cmd : this->items_)
        {
            if (cmd.name == args.item.name)
            {
                this->userCommands_[cmd.name] = cmd;
                break;
            }
        }

        int maxSpaces = 0;

        for (const Command &cmd : this->items_)
        {
            auto localMaxSpaces = cmd.name.count(' ');
            if (localMaxSpaces > maxSpaces)
            {
                maxSpaces = localMaxSpaces;
            }
        }

        this->maxSpaces_ = maxSpaces;
    };
    this->items_.itemInserted.connect(addFirstMatchToMap);
    this->items_.itemRemoved.connect(addFirstMatchToMap);

    // Initialize setting manager for commands.json
    auto path = combinePath(paths.settingsDirectory, "commands.json");
    this->sm_ = std::make_shared<pajlada::Settings::SettingManager>();
    this->sm_->setPath(path.toStdString());
    this->sm_->setBackupEnabled(true);
    this->sm_->setBackupSlots(9);

    // Delayed initialization of the setting storing all commands
    this->commandsSetting_.reset(
        new pajlada::Settings::Setting<std::vector<Command>>("/commands",
                                                             this->sm_));

    // Update the setting when the vector of commands has been updated (most
    // likely from the settings dialog)
    this->items_.delayedItemsChanged.connect([this] {
        this->commandsSetting_->setValue(this->items_.raw());
    });

    // Load commands from commands.json
    this->sm_->load();

    // Add loaded commands to our vector of commands (which will update the map
    // of commands)
    for (const auto &command : this->commandsSetting_->getValue())
    {
        this->items_.append(command);
    }

    /// Deprecated commands

    auto blockLambda = [](const auto &words, auto channel) {
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage("Usage: /block <user>"));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to block someone!"));
            return "";
        }

        auto target = words.at(1);

        getHelix()->getUserByName(
            target,
            [currentUser, channel, target](const HelixUser &targetUser) {
                getApp()->accounts->twitch.getCurrent()->blockUser(
                    targetUser.id,
                    [channel, target, targetUser] {
                        channel->addMessage(makeSystemMessage(
                            QString("You successfully blocked user %1")
                                .arg(target)));
                    },
                    [channel, target] {
                        channel->addMessage(makeSystemMessage(
                            QString("User %1 couldn't be blocked, an unknown "
                                    "error occurred!")
                                .arg(target)));
                    });
            },
            [channel, target] {
                channel->addMessage(
                    makeSystemMessage(QString("User %1 couldn't be blocked, no "
                                              "user with that name found!")
                                          .arg(target)));
            });

        return "";
    };

    auto unblockLambda = [](const auto &words, auto channel) {
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage("Usage: /unblock <user>"));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to unblock someone!"));
            return "";
        }

        auto target = words.at(1);

        getHelix()->getUserByName(
            target,
            [currentUser, channel, target](const auto &targetUser) {
                getApp()->accounts->twitch.getCurrent()->unblockUser(
                    targetUser.id,
                    [channel, target, targetUser] {
                        channel->addMessage(makeSystemMessage(
                            QString("You successfully unblocked user %1")
                                .arg(target)));
                    },
                    [channel, target] {
                        channel->addMessage(makeSystemMessage(
                            QString("User %1 couldn't be unblocked, an unknown "
                                    "error occurred!")
                                .arg(target)));
                    });
            },
            [channel, target] {
                channel->addMessage(
                    makeSystemMessage(QString("User %1 couldn't be unblocked, "
                                              "no user with that name found!")
                                          .arg(target)));
            });

        return "";
    };

    this->registerCommand(
        "/ignore", [blockLambda](const auto &words, auto channel) {
            channel->addMessage(makeSystemMessage(
                "Ignore command has been renamed to /block, please use it from "
                "now on as /ignore is going to be removed soon."));
            blockLambda(words, channel);
            return "";
        });

    this->registerCommand(
        "/unignore", [unblockLambda](const auto &words, auto channel) {
            channel->addMessage(makeSystemMessage(
                "Unignore command has been renamed to /unblock, please use it "
                "from now on as /unignore is going to be removed soon."));
            unblockLambda(words, channel);
            return "";
        });

    this->registerCommand("/massban", [](const auto &words, auto channel) {
        auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            channel->addMessage(makeSystemMessage(
                "You must be logged in to perform this action!"));
            return "";
        }

        TwitchChannel *twitchChannel =
            dynamic_cast<TwitchChannel *>(channel.get());

        bool isModOrBroadcaster =
            twitchChannel ? twitchChannel->hasModRights() : false;

        if (!isModOrBroadcaster)
        {
            channel->addMessage(
                makeSystemMessage("You don't have permissions to execute this "
                                  "command in this channel!"));
            return "";
        }

        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage(
                "Usage: /massban [username1] [username2] ..."));
            return "";
        }

        for (int i = 1; i < words.size(); i++)
        {
            QString channelTarget = words.at(i);
            stripChannelName(channelTarget);
            if (i == 1)
            {
                channel->sendMessage("/ban " + channelTarget);
            }
            else
            {
                QTimer::singleShot((i - 1) * 1000, [channel, channelTarget]() {
                    channel->sendMessage("/ban " + channelTarget);
                });
            }
        }

        return "";
    });

    this->registerCommand("/massunban", [](const auto &words, auto channel) {
        auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            channel->addMessage(makeSystemMessage(
                "You must be logged in to perform this action!"));
            return "";
        }

        TwitchChannel *twitchChannel =
            dynamic_cast<TwitchChannel *>(channel.get());

        bool isModOrBroadcaster =
            twitchChannel ? twitchChannel->hasModRights() : false;

        if (!isModOrBroadcaster)
        {
            channel->addMessage(
                makeSystemMessage("You don't have permissions to execute this "
                                  "command in this channel!"));
            return "";
        }

        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage(
                "Usage: /massunban [username1] [username2] ..."));
            return "";
        }

        for (int i = 1; i < words.size(); i++)
        {
            QString channelTarget = words.at(i);
            stripChannelName(channelTarget);
            if (i == 1)
            {
                channel->sendMessage("/unban " + channelTarget);
            }
            else
            {
                QTimer::singleShot((i - 1) * 1000, [channel, channelTarget]() {
                    channel->sendMessage("/unban " + channelTarget);
                });
            }
        }

        return "";
    });

    this->registerCommand("/nuke", [this](const auto &words, auto channel) {
        auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            channel->addMessage(makeSystemMessage(
                "You must be logged in to perform this action!"));
            return "";
        }

        TwitchChannel *twitchChannel =
            dynamic_cast<TwitchChannel *>(channel.get());

        bool isModOrBroadcaster =
            twitchChannel ? twitchChannel->hasModRights() : false;

        if (!isModOrBroadcaster)
        {
            channel->addMessage(
                makeSystemMessage("You don't have permissions to execute this "
                                  "command in this channel!"));
            return "";
        }

        if (words.size() < 3)
        {
            channel->addMessage(
                makeSystemMessage("Usage: /nuke my phrase [-r=<number><s|m>] "
                                  "[delete|ban|<number>[s|m|h|d|w]]"));
            return "";
        }

        QString phrase = words.mid(1, words.size() - 2).join(" ");
        QString action = words.at(words.size() - 1);
        LimitedQueueSnapshot<MessagePtr> snapshot =
            channel->getMessageSnapshot();
        QString seconds;
        QString minutes;
        bool persistence = false;

        if (words.at(words.size() - 2).startsWith("-r="))
        {
            QStringList persistence_list =
                words.at(words.size() - 2).split("-r=");
            phrase = words.mid(1, words.size() - 3).join(" ");

            if (persistence_list.size() == 2)
            {
                QString persistenceTime = persistence_list.at(1);

                if (persistenceTime.endsWith("s"))
                {
                    seconds =
                        persistenceTime.left(persistenceTime.lastIndexOf("s"));
                    persistence = true;
                }

                else if (persistenceTime.endsWith("m"))
                {
                    minutes =
                        persistenceTime.left(persistenceTime.lastIndexOf("m"));
                    persistence = true;
                }

                else
                {
                    channel->addMessage(
                        makeSystemMessage("Ignoring persistence since the "
                                          "introduced value is invalid"));
                    persistence = false;
                }
            }
        }

        static QRegularExpression timeoutRegex("(\\d+)([smhdw])");
        auto timeoutMatch = timeoutRegex.match(action);

        if (action != "ban" && action != "delete" && !timeoutMatch.hasMatch())
        {
            channel->addMessage(
                makeSystemMessage("Invalid action. Valid actions: ban, delete, "
                                  "<x>s, <x>m, <x>h, <x>d, <x>w"));
            channel->addMessage(
                makeSystemMessage("Example: /nuke my phrase 10m"));
            return "";
        }

        int nuked = 0;
        int amount = 0;
        QStringList usernames;

        if (timeoutMatch.hasMatch())
        {
            constexpr int minute = 60;
            constexpr int hour = 60 * minute;
            constexpr int day = 24 * hour;
            constexpr int week = 7 * day;

            amount = timeoutMatch.captured(1).toInt();
            QString unit = timeoutMatch.captured(2);

            if (amount <= 0)
            {
                channel->addMessage(makeSystemMessage(
                    "Invalid timeout value. It has to be higher than 0."));
                return "";
            }

            if (unit == "m")
            {
                amount *= minute;
            }
            else if (unit == "h")
            {
                amount *= hour;
            }
            else if (unit == "d")
            {
                amount *= day;
            }
            else if (unit == "w")
            {
                amount *= week;
            }
        }

        for (size_t i = 0; i < snapshot.size(); ++i)
        {
            MessagePtr message = snapshot[i];

            if (!message->loginName.isEmpty())
            {
                if (message->loginName == channel->getName() || message->isMod)
                {
                    continue;
                }

                if (message->messageText.contains(phrase, Qt::CaseInsensitive))
                {
                    if (action == "delete")
                    {
                        std::cout << message->messageText.toStdString()
                                  << std::endl;
                        QTimer::singleShot(
                            (nuked + 1) * 1000, [channel, message]() {
                                channel->sendMessage("/delete " + message->id);
                            });

                        nuked++;
                    }

                    if (action == "ban")
                    {
                        if (usernames.indexOf(message->loginName) == -1)
                        {
                            QTimer::singleShot(
                                (nuked + 1) * 1000, [channel, message]() {
                                    channel->sendMessage("/ban " +
                                                         message->loginName);
                                });

                            usernames.append(message->loginName);
                            nuked++;
                        }
                    }

                    if (timeoutMatch.hasMatch())
                    {
                        if (usernames.indexOf(message->loginName) == -1)
                        {
                            QTimer::singleShot(
                                (nuked + 1) * 1000,
                                [channel, message, amount]() {
                                    channel->sendMessage(
                                        QString("/timeout %1 %2")
                                            .arg(message->loginName)
                                            .arg(amount));
                                });

                            usernames.append(message->loginName);
                            nuked++;
                        }
                    }
                }
            }
        }

        if (!seconds.isEmpty())
        {
            if (seconds.toInt())
            {
                QTimer::singleShot(seconds.toInt() * 1000, [this]() {
                    this->privateMessageReceivedConnection.disconnect();
                });
            }
            else
            {
                channel->addMessage(
                    makeSystemMessage("Ignoring persistence since the "
                                      "introduced value is invalid"));
                return "";
            }
        }

        else if (!minutes.isEmpty())
        {
            if (minutes.toInt())
            {
                QTimer::singleShot(minutes.toInt() * 60 * 1000, [this]() {
                    this->privateMessageReceivedConnection.disconnect();
                });
            }
            else
            {
                channel->addMessage(
                    makeSystemMessage("Ignoring persistence since the "
                                      "introduced value is invalid"));
                return "";
            }
        }

        static QRegularExpression idRegex(
            "(id=)(\\w{8}-\\w{4}-\\w{4}-\\w{4}-\\w{12})");
        static QRegularExpression modRegex("(mod=)(\\w{1})");
        QString channelName = channel->getName();
        stripChannelName(channelName);

        if (persistence)
        {
            this->privateMessageReceivedConnection =
                this->privateMessageReceivedSignal.connect(
                    [this, channelName, phrase, action, amount, timeoutMatch,
                     &usernames, channel](Communi::IrcPrivateMessage &msg) {
                        QTimer *timer = new QTimer;
                        timer->setSingleShot(true);

                        QString messageData = QString(msg.toData());
                        auto modMatch = modRegex.match(messageData);

                        if (msg.nick() == channelName ||
                            (modMatch.captured(2) == "1" ? true : false))
                        {
                            return "";
                        }

                        QString target = msg.target();
                        stripChannelName(target);

                        auto idMatch = idRegex.match(messageData);

                        if (channelName == target)
                        {
                            QString message = msg.content();

                            if (message.contains(phrase, Qt::CaseInsensitive))
                            {
                                QString id = idMatch.captured(2);
                                QString sender = msg.nick();

                                if (action == "ban")
                                {
                                    if (usernames.indexOf(sender) == -1)
                                    {
                                        channel->sendMessage("/ban " + sender);
                                        usernames.append(sender);
                                    }
                                }

                                if (action == "delete")
                                {
                                    channel->sendMessage("/delete " + id);
                                }

                                if (timeoutMatch.hasMatch())
                                {
                                    channel->sendMessage(
                                        QString("/timeout %1 %2")
                                            .arg(sender)
                                            .arg(amount));
                                }
                            }
                        }

                        return "";
                    });
        }

        return "";
    });

    this->registerCommand("/follow", [](const auto &words, auto channel) {
        auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to follow someone!"));
            return "";
        }

        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage("Usage: /follow [user]"));
            return "";
        }

        auto followHash = pajlada::Settings::Setting<QString>::get(
            "/accounts/uid" + currentUser->getUserId().toStdString() +
            "/followHash");
        auto followToken = pajlada::Settings::Setting<QString>::get(
            "/accounts/uid" + currentUser->getUserId().toStdString() +
            "/followToken");

        if (followHash.isEmpty() || followToken.isEmpty())
        {
            channel->addMessage(
                makeSystemMessage("In order to follow someone you have to "
                                  "introduce the needed hashes and tokens in: "
                                  "Settings ➜ Accounts ➜ Accounts settings"));
            return "";
        }

        auto target = words.at(1);

        getHelix()->getUserByName(
            target,
            [currentUser, channel, target, followHash,
             followToken](const auto &targetUser) {
                getHelix()->followUser(
                    currentUser->getUserId(), targetUser.id, followHash,
                    followToken,
                    [channel, target]() {
                        channel->addMessage(makeSystemMessage(
                            "You successfully followed " + target));
                    },
                    [channel, target]() {
                        channel->addMessage(makeSystemMessage(
                            QString(
                                "User %1 could not be followed, an "
                                "error occurred! Please check that you have "
                                "introduced the correct values in: Settings ➜ "
                                "Accounts ➜ Accounts settings")
                                .arg(target)));
                    });
            },
            [channel, target] {
                channel->addMessage(
                    makeSystemMessage(QString("User %1 could not be followed, "
                                              "no user with that name found!")
                                          .arg(target)));
            });

        return "";
    });

    this->registerCommand("/unfollow", [](const auto &words, auto channel) {
        auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            channel->addMessage(makeSystemMessage(
                "You must be logged in to unfollow someone!"));
            return "";
        }

        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage("Usage: /unfollow [user]"));
            return "";
        }

        auto unfollowHash = pajlada::Settings::Setting<QString>::get(
            "/accounts/uid" + currentUser->getUserId().toStdString() +
            "/unfollowHash");
        auto followToken = pajlada::Settings::Setting<QString>::get(
            "/accounts/uid" + currentUser->getUserId().toStdString() +
            "/followToken");

        if (unfollowHash.isEmpty() || followToken.isEmpty())
        {
            channel->addMessage(
                makeSystemMessage("In order to unfollow someone you have to "
                                  "introduce the needed hashes and tokens in: "
                                  "Settings ➜ Accounts ➜ Accounts settings"));
            return "";
        }

        auto target = words.at(1);

        getHelix()->getUserByName(
            target,
            [currentUser, channel, target, unfollowHash,
             followToken](const auto &targetUser) {
                getHelix()->unfollowUser(
                    currentUser->getUserId(), targetUser.id, unfollowHash,
                    followToken,
                    [channel, target]() {
                        channel->addMessage(makeSystemMessage(
                            "You successfully unfollowed " + target));
                    },
                    [channel, target]() {
                        channel->addMessage(makeSystemMessage(
                            QString("User %1 could not be unfollowed, an error "
                                    "occurred! Please check that you have "
                                    "introduced the correct values in: "
                                    "Settings ➜ Accounts ➜ Accounts settings")
                                .arg(target)));
                    });
            },
            [channel, target] {
                channel->addMessage(makeSystemMessage(
                    QString("User %1 could not be unfollowed, "
                            "no user with that name found!")
                        .arg(target)));
            });

        return "";
    });

    /// Supported commands

    this->registerCommand(
        "/debug-args", [](const auto & /*words*/, auto channel) {
            QString msg = QApplication::instance()->arguments().join(' ');

            channel->addMessage(makeSystemMessage(msg));

            return "";
        });

    this->registerCommand("/uptime", [](const auto & /*words*/, auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /uptime command only works in Twitch Channels"));
            return "";
        }

        const auto &streamStatus = twitchChannel->accessStreamStatus();

        QString messageText =
            streamStatus->live ? streamStatus->uptime : "Channel is not live.";

        channel->addMessage(makeSystemMessage(messageText));

        return "";
    });

    this->registerCommand("/block", blockLambda);

    this->registerCommand("/unblock", unblockLambda);

    this->registerCommand("/user", [](const auto &words, auto channel) {
        if (words.size() < 2)
        {
            channel->addMessage(
                makeSystemMessage("Usage: /user <user> [channel]"));
            return "";
        }
        QString userName = words[1];
        stripUserName(userName);

        QString channelName = channel->getName();

        if (words.size() > 2)
        {
            channelName = words[2];
            stripChannelName(channelName);
        }
        openTwitchUsercard(channelName, userName);

        return "";
    });

    this->registerCommand("/usercard", [](const auto &words, auto channel) {
        if (words.size() < 2)
        {
            channel->addMessage(
                makeSystemMessage("Usage: /usercard <user> [channel]"));
            return "";
        }

        QString userName = words[1];
        stripUserName(userName);

        if (words.size() > 2)
        {
            QString channelName = words[2];
            stripChannelName(channelName);

            ChannelPtr channelTemp =
                getApp()->twitch2->getChannelOrEmpty(channelName);

            if (channelTemp->isEmpty())
            {
                channel->addMessage(makeSystemMessage(
                    "A usercard can only be displayed for a channel that is "
                    "currently opened in Chatterino."));
                return "";
            }

            channel = channelTemp;
        }

        auto *userPopup = new UserInfoPopup(
            getSettings()->autoCloseUserPopup,
            static_cast<QWidget *>(&(getApp()->windows->getMainWindow())));
        userPopup->setData(userName, channel);
        userPopup->move(QCursor::pos());
        userPopup->show();
        return "";
    });

    this->registerCommand(
        "/chatters", [](const auto & /*words*/, auto channel) {
            auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

            if (twitchChannel == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "The /chatters command only works in Twitch Channels"));
                return "";
            }

            channel->addMessage(makeSystemMessage(
                QString("Chatter count: %1")
                    .arg(localizeNumbers(twitchChannel->chatterCount()))));

            return "";
        });

    this->registerCommand("/reset", [](const auto & /*words*/, auto channel) {
        boost::filesystem::remove_all(
            (getPaths()->cacheDirectory()).toStdString());
        qApp->quit();
        QProcess::startDetached(qApp->arguments()[0], qApp->arguments());

        return "";
    });

    this->registerCommand("/clip", [](const auto & /*words*/, auto channel) {
        if (const auto type = channel->getType();
            type != Channel::Type::Twitch &&
            type != Channel::Type::TwitchWatching)
        {
            return "";
        }

        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

        twitchChannel->createClip();

        return "";
    });

    this->registerCommand("/marker", [](const QStringList &words,
                                        auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /marker command only works in Twitch channels"));
            return "";
        }

        // Avoid Helix calls without Client ID and/or OAuth Token
        if (getApp()->accounts->twitch.getCurrent()->isAnon())
        {
            channel->addMessage(makeSystemMessage(
                "You need to be logged in to create stream markers!"));
            return "";
        }

        // Exact same message as in webchat
        if (!twitchChannel->isLive())
        {
            channel->addMessage(makeSystemMessage(
                "You can only add stream markers during live streams. Try "
                "again when the channel is live streaming."));
            return "";
        }

        auto arguments = words;
        arguments.removeFirst();

        getHelix()->createStreamMarker(
            // Limit for description is 140 characters, webchat just crops description
            // if it's >140 characters, so we're doing the same thing
            twitchChannel->roomId(), arguments.join(" ").left(140),
            [channel, arguments](const HelixStreamMarker &streamMarker) {
                channel->addMessage(makeSystemMessage(
                    QString("Successfully added a stream marker at %1%2")
                        .arg(formatTime(streamMarker.positionSeconds))
                        .arg(streamMarker.description.isEmpty()
                                 ? ""
                                 : QString(": \"%1\"")
                                       .arg(streamMarker.description))));
            },
            [channel](auto error) {
                QString errorMessage("Failed to create stream marker - ");

                switch (error)
                {
                    case HelixStreamMarkerError::UserNotAuthorized: {
                        errorMessage +=
                            "you don't have permission to perform that action.";
                    }
                    break;

                    case HelixStreamMarkerError::UserNotAuthenticated: {
                        errorMessage += "you need to re-authenticate.";
                    }
                    break;

                    // This would most likely happen if the service is down, or if the JSON payload returned has changed format
                    case HelixStreamMarkerError::Unknown:
                    default: {
                        errorMessage += "an unknown error occurred.";
                    }
                    break;
                }

                channel->addMessage(makeSystemMessage(errorMessage));
            });

        return "";
    });

    this->registerCommand(
        "/streamlink", [](const QStringList &words, ChannelPtr channel) {
            QString target(words.size() < 2 ? channel->getName() : words[1]);

            if (words.size() < 2 &&
                (!channel->isTwitchChannel() || channel->isEmpty()))
            {
                channel->addMessage(makeSystemMessage(
                    "Usage: /streamlink <channel>. You can also use the "
                    "command without arguments in any Twitch channel to open "
                    "it in streamlink."));
                return "";
            }

            stripChannelName(target);
            channel->addMessage(makeSystemMessage(
                QString("Opening %1 in streamlink...").arg(target)));
            openStreamlinkForChannel(target);

            return "";
        });

    this->registerCommand(
        "/popout", [](const QStringList &words, ChannelPtr channel) {
            QString target(words.size() < 2 ? channel->getName() : words[1]);

            if (words.size() < 2 &&
                (!channel->isTwitchChannel() || channel->isEmpty()))
            {
                channel->addMessage(makeSystemMessage(
                    "Usage: /popout <channel>. You can also use the command "
                    "without arguments in any Twitch channel to open its "
                    "popout chat."));
                return "";
            }

            stripChannelName(target);
            QDesktopServices::openUrl(
                QUrl(QString("https://www.twitch.tv/popout/%1/chat?popout=")
                         .arg(target)));

            return "";
        });

    this->registerCommand("/clearmessages", [](const auto & /*words*/,
                                               ChannelPtr channel) {
        auto *currentPage = dynamic_cast<SplitContainer *>(
            getApp()->windows->getMainWindow().getNotebook().getSelectedPage());

        currentPage->getSelectedSplit()->getChannelView().clearMessages();
        return "";
    });

    this->registerCommand("/settitle", [](const QStringList &words,
                                          ChannelPtr channel) {
        if (words.size() < 2)
        {
            channel->addMessage(
                makeSystemMessage("Usage: /settitle <stream title>"));
            return "";
        }
        if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
        {
            auto status = twitchChannel->accessStreamStatus();
            auto title = words.mid(1).join(" ");
            getHelix()->updateChannel(
                twitchChannel->roomId(), "", "", title,
                [channel, title](NetworkResult) {
                    channel->addMessage(makeSystemMessage(
                        QString("Updated title to %1").arg(title)));
                },
                [channel] {
                    channel->addMessage(
                        makeSystemMessage("Title update failed! Are you "
                                          "missing the required scope?"));
                });
        }
        else
        {
            channel->addMessage(makeSystemMessage(
                "Unable to set title of non-Twitch channel."));
        }
        return "";
    });
    this->registerCommand("/setgame", [](const QStringList &words,
                                         const ChannelPtr channel) {
        if (words.size() < 2)
        {
            channel->addMessage(
                makeSystemMessage("Usage: /setgame <stream game>"));
            return "";
        }
        if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
        {
            const auto gameName = words.mid(1).join(" ");

            getHelix()->searchGames(
                gameName,
                [channel, twitchChannel,
                 gameName](const std::vector<HelixGame> &games) {
                    if (games.empty())
                    {
                        channel->addMessage(
                            makeSystemMessage("Game not found."));
                        return;
                    }

                    auto matchedGame = games.at(0);

                    if (games.size() > 1)
                    {
                        // NOTE: Improvements could be made with 'fuzzy string matching' code here
                        // attempt to find the best looking game by comparing exactly with lowercase values
                        for (const auto &game : games)
                        {
                            if (game.name.toLower() == gameName.toLower())
                            {
                                matchedGame = game;
                                break;
                            }
                        }
                    }

                    auto status = twitchChannel->accessStreamStatus();
                    getHelix()->updateChannel(
                        twitchChannel->roomId(), matchedGame.id, "", "",
                        [channel, games, matchedGame](const NetworkResult &) {
                            channel->addMessage(
                                makeSystemMessage(QString("Updated game to %1")
                                                      .arg(matchedGame.name)));
                        },
                        [channel] {
                            channel->addMessage(makeSystemMessage(
                                "Game update failed! Are you "
                                "missing the required scope?"));
                        });
                },
                [channel] {
                    channel->addMessage(
                        makeSystemMessage("Failed to look up game."));
                });
        }
        else
        {
            channel->addMessage(
                makeSystemMessage("Unable to set game of non-Twitch channel."));
        }
        return "";
    });

    this->registerCommand("/openurl", [](const QStringList &words,
                                         const ChannelPtr channel) {
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage("Usage: /openurl <URL>"));
            return "";
        }

        QUrl url = QUrl::fromUserInput(words.mid(1).join(" "));
        if (!url.isValid())
        {
            channel->addMessage(makeSystemMessage("Invalid URL specified."));
            return "";
        }

        bool res = false;
        if (supportsIncognitoLinks() && getSettings()->openLinksIncognito)
        {
            res = openLinkIncognito(url.toString(QUrl::FullyEncoded));
        }
        else
        {
            res = QDesktopServices::openUrl(url);
        }

        if (!res)
        {
            channel->addMessage(makeSystemMessage("Could not open URL."));
        }

        return "";
    });
    this->registerCommand(
        "/delete", [](const QStringList &words, ChannelPtr channel) -> QString {
            // This is a wrapper over the standard Twitch /delete command
            // We use this to ensure the user gets better error messages for missing or malformed arguments
            if (words.size() < 2)
            {
                channel->addMessage(
                    makeSystemMessage("Usage: /delete <msg-id> - Deletes the "
                                      "specified message."));
                return "";
            }

            auto messageID = words.at(1);
            auto uuid = QUuid(messageID);
            if (uuid.isNull())
            {
                // The message id must be a valid UUID
                channel->addMessage(makeSystemMessage(
                    QString("Invalid msg-id: \"%1\"").arg(messageID)));
                return "";
            }

            auto msg = channel->findMessage(messageID);
            if (msg != nullptr)
            {
                if (msg->loginName == channel->getName() &&
                    !channel->isBroadcaster())
                {
                    channel->addMessage(makeSystemMessage(
                        "You cannot delete the broadcaster's messages unless "
                        "you are the broadcaster."));
                    return "";
                }
            }

            return QString("/delete ") + messageID;
        });

    this->registerCommand("/raw", [](const QStringList &words, ChannelPtr) {
        getApp()->twitch2->sendRawMessage(words.mid(1).join(" "));
        return "";
    });
}

void CommandController::save()
{
    this->sm_->save();
}

CommandModel *CommandController::createModel(QObject *parent)
{
    CommandModel *model = new CommandModel(parent);
    model->initialize(&this->items_);

    return model;
}

QString CommandController::execCommand(const QString &textNoEmoji,
                                       ChannelPtr channel, bool dryRun)
{
    QString text = getApp()->emotes->emojis.replaceShortCodes(textNoEmoji);
    QStringList words = text.split(' ', QString::SkipEmptyParts);

    if (words.length() == 0)
    {
        return text;
    }

    QString commandName = words[0];

    // works in a valid Twitch channel and /whispers, etc...
    if (!dryRun && channel->isTwitchChannel())
    {
        if (whisperCommands.contains(commandName, Qt::CaseInsensitive))
        {
            if (words.length() > 2)
            {
                appendWhisperMessageWordsLocally(words);
                sendWhisperMessage(text);
            }

            return "";
        }
    }

    {
        // check if user command exists
        const auto it = this->userCommands_.find(commandName);
        if (it != this->userCommands_.end())
        {
            text = getApp()->emotes->emojis.replaceShortCodes(
                this->execCustomCommand(words, it.value(), dryRun, channel));

            words = text.split(' ', QString::SkipEmptyParts);

            if (words.length() == 0)
            {
                return text;
            }

            commandName = words[0];
        }
    }

    // works only in a valid Twitch channel
    if (!dryRun && channel->isTwitchChannel())
    {
        // check if command exists
        const auto it = this->commands_.find(commandName);
        if (it != this->commands_.end())
        {
            return it.value()(words, channel);
        }
    }

    auto maxSpaces = std::min(this->maxSpaces_, words.length() - 1);
    for (int i = 0; i < maxSpaces; ++i)
    {
        commandName += ' ' + words[i + 1];

        const auto it = this->userCommands_.find(commandName);
        if (it != this->userCommands_.end())
        {
            return this->execCustomCommand(words, it.value(), dryRun, channel);
        }
    }

    return text;
}

void CommandController::registerCommand(QString commandName,
                                        CommandFunction commandFunction)
{
    assert(!this->commands_.contains(commandName));

    this->commands_[commandName] = commandFunction;

    this->commandAutoCompletions_.append(commandName);
}

QString CommandController::execCustomCommand(const QStringList &words,
                                             const Command &command,
                                             bool dryRun, ChannelPtr channel,
                                             std::map<QString, QString> context)
{
    QString result;

    static QRegularExpression parseCommand(
        R"((^|[^{])({{)*{(\d+\+?|([a-zA-Z.-]+)(?:;(.+?))?)})");

    int lastCaptureEnd = 0;

    auto globalMatch = parseCommand.globalMatch(command.func);
    int matchOffset = 0;

    while (true)
    {
        QRegularExpressionMatch match =
            parseCommand.match(command.func, matchOffset);

        if (!match.hasMatch())
        {
            break;
        }

        result += command.func.mid(lastCaptureEnd,
                                   match.capturedStart() - lastCaptureEnd + 1);

        lastCaptureEnd = match.capturedEnd();
        matchOffset = lastCaptureEnd - 1;

        QString wordIndexMatch = match.captured(3);

        bool plus = wordIndexMatch.at(wordIndexMatch.size() - 1) == '+';
        wordIndexMatch = wordIndexMatch.replace("+", "");

        bool ok;
        int wordIndex = wordIndexMatch.replace("=", "").toInt(&ok);
        if (!ok || wordIndex == 0)
        {
            auto varName = match.captured(4);
            auto altText = match.captured(5);  // alt text or empty string

            auto var = COMMAND_VARS.find(varName);

            if (var != COMMAND_VARS.end())
            {
                result += var->second(altText, channel);
            }
            else
            {
                auto it = context.find(varName);
                if (it != context.end())
                {
                    result += it->second.isEmpty() ? altText : it->second;
                }
                else
                {
                    result += "{" + match.captured(3) + "}";
                }
            }
            continue;
        }

        if (words.length() <= wordIndex)
        {
            continue;
        }

        if (plus)
        {
            bool first = true;
            for (int i = wordIndex; i < words.length(); i++)
            {
                if (!first)
                {
                    result += " ";
                }
                result += words[i];
                first = false;
            }
        }
        else
        {
            result += words[wordIndex];
        }
    }

    result += command.func.mid(lastCaptureEnd);

    if (result.size() > 0 && result.at(0) == '{')
    {
        result = result.mid(1);
    }

    auto res = result.replace("{{", "{");

    if (dryRun || !appendWhisperMessageStringLocally(res))
    {
        return res;
    }
    else
    {
        sendWhisperMessage(res);
        return "";
    }
}

QStringList CommandController::getDefaultTwitchCommandList()
{
    return this->commandAutoCompletions_;
}

void CommandController::newMessageReceived(Communi::IrcPrivateMessage &msg)
{
    this->privateMessageReceivedSignal.invoke(msg);
}

}  // namespace chatterino
