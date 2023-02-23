#include "HomiesEmotes.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QThread>

namespace chatterino {
namespace {
    const QString CHANNEL_HAS_NO_EMOTES(
        "This channel has no Homies channel emotes.");

    const QString emoteLinkFormat("https://7tv.app/emotes/%1");

    Url getEmoteLink(const EmoteId &id, const QString &emoteScale)
    {
        const QString urlTemplate("https://itzalex.github.io/emote/%1/%2");

        return {urlTemplate.arg(id.string, emoteScale)};
    }

    EmotePtr cachedOrMake(Emote &&emote, const EmoteId &id)
    {
        static std::unordered_map<EmoteId, std::weak_ptr<const Emote>> cache;
        static std::mutex mutex;

        return cachedOrMakeEmotePtr(std::move(emote), cache, mutex, id);
    }

    struct CreateEmoteResult {
        EmoteId id;
        EmoteName name;
        Emote emote;
    };

    CreateEmoteResult createEmote(QJsonValue jsonEmote, bool isGlobal)
    {
        auto id = EmoteId{jsonEmote.toObject().value("id").toString()};
        auto name = EmoteName{jsonEmote.toObject().value("name").toString()};
        auto author =
            EmoteAuthor{jsonEmote.toObject().value("author").toString()};
        bool zeroWidth = jsonEmote.toObject().value("zerowidth").toBool();

        auto emote = Emote(
            {name,
             ImageSet{Image::fromUrl(getEmoteLink(id, "1x"), 1),
                      Image::fromUrl(getEmoteLink(id, "2x"), 0.66),
                      Image::fromUrl(getEmoteLink(id, "3x"), 0.33)},
             Tooltip{QString("%1<br>%2 Homies Emote<br>By: %3")
                         .arg(name.string, (isGlobal ? "Global" : "Channel"),
                              author.string)},
             Url{emoteLinkFormat.arg(id.string)}, zeroWidth});

        auto result = CreateEmoteResult({id, name, emote});
        return result;
    }

    std::pair<Outcome, EmoteMap> parseGlobalEmotes(
        const QJsonArray &jsonEmotes, const EmoteMap &currentEmotes)
    {
        auto emotes = EmoteMap();

        for (const auto &jsonEmote : jsonEmotes)
        {
            auto emote = createEmote(jsonEmote, true);
            emotes[emote.name] =
                cachedOrMakeEmotePtr(std::move(emote.emote), currentEmotes);
        }

        return {Success, std::move(emotes)};
    }

    EmoteMap parseChannelEmotes(const QJsonObject &root,
                                const QString &channelName)
    {
        auto emotes = EmoteMap();

        auto jsonEmotes = root.value("emotes").toArray();
        for (auto jsonEmote_ : jsonEmotes)
        {
            auto jsonEmote = jsonEmote_.toObject();

            auto emote = createEmote(jsonEmote, false);

            emotes[emote.name] = cachedOrMake(std::move(emote.emote), emote.id);
        }

        return emotes;
    }
}  // namespace

HomiesEmotes::HomiesEmotes()
    : global_(std::make_shared<EmoteMap>())
{
}

std::shared_ptr<const EmoteMap> HomiesEmotes::emotes() const
{
    return this->global_.get();
}

boost::optional<EmotePtr> HomiesEmotes::emote(const EmoteName &name) const
{
    auto emotes = this->global_.get();
    auto it = emotes->find(name);

    if (it == emotes->end())
        return boost::none;
    return it->second;
}

void HomiesEmotes::loadEmotes()
{
    if (!Settings::instance().enableHomiesGlobalEmotes)
    {
        this->global_.set(EMPTY_EMOTE_MAP);
        return;
    }

    NetworkRequest(apiUrl)
        .onSuccess([this](NetworkResult result) -> Outcome {
            QJsonArray parsedEmotes = result.parseJson()
                                          .value("data")
                                          .toObject()
                                          .value("global_emotes")
                                          .toArray();

            auto pair = parseGlobalEmotes(parsedEmotes, *this->global_.get());
            if (pair.first)
                this->global_.set(
                    std::make_shared<EmoteMap>(std::move(pair.second)));
            return pair.first;
        })
        .execute();
}

void HomiesEmotes::loadChannel(std::weak_ptr<Channel> channel,
                               const QString &channelId,
                               std::function<void(EmoteMap &&)> callback,
                               bool manualRefresh)
{
    NetworkRequest(apiUrl)
        .onSuccess([callback = std::move(callback), channel, channelId,
                    manualRefresh](NetworkResult result) -> Outcome {
            QJsonObject parsedEmotes = result.parseJson()
                                           .value("data")
                                           .toObject()
                                           .value("channel_emotes")
                                           .toObject()
                                           .value(channelId)
                                           .toObject();

            auto emoteMap = parseChannelEmotes(parsedEmotes, channelId);
            bool hasEmotes = !emoteMap.empty();

            if (hasEmotes)
            {
                callback(std::move(emoteMap));
            }
            if (auto shared = channel.lock(); manualRefresh)
            {
                if (hasEmotes)
                {
                    shared->addMessage(
                        makeSystemMessage("Homies channel emotes reloaded."));
                }
                else
                {
                    shared->addMessage(
                        makeSystemMessage(CHANNEL_HAS_NO_EMOTES));
                }
            }
            return Success;
        })
        .onError([channelId, channel, manualRefresh](NetworkResult result) {
            auto shared = channel.lock();
            if (!shared)
                return;
            if (result.status() == 400)
            {
                if (manualRefresh)
                    shared->addMessage(
                        makeSystemMessage(CHANNEL_HAS_NO_EMOTES));
            }
            else
            {
                shared->addMessage(
                    makeSystemMessage("Failed to fetch Homies channel "
                                      "emotes. (unknown error)"));
            }
        })
        .execute();
}

}  // namespace chatterino