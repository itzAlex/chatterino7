#include "HomiesBadges.hpp"

#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "messages/Emote.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QUrl>

namespace chatterino {
void HomiesBadges::initialize(Settings &settings, Paths &paths)
{
    this->loadHomiesBadges();
}

HomiesBadges::HomiesBadges()
{
}

boost::optional<EmotePtr> HomiesBadges::getBadge(const UserId &id)
{
    std::shared_lock lock(this->mutex_);

    auto it = badgeMap.find(id.string);
    if (it != badgeMap.end())
    {
        return emotes[it->second];
    }
    return boost::none;
}

boost::optional<EmotePtr> HomiesBadges::getBadge2(const UserId &id)
{
    std::shared_lock lock(this->mutex_);

    auto it = badgeMap2.find(id.string);
    if (it != badgeMap2.end())
    {
        return emotes2[it->second];
    }
    return boost::none;
}

boost::optional<EmotePtr> HomiesBadges::getBadge3(const UserId &id)
{
    std::shared_lock lock(this->mutex_);

    auto it = badgeMap3.find(id.string);
    if (it != badgeMap3.end())
    {
        return emotes3[it->second];
    }
    return boost::none;
}

void HomiesBadges::loadHomiesBadges()
{
    badgeMap.clear();
    badgeMap2.clear();
    badgeMap3.clear();
    emotes.clear();
    emotes2.clear();
    emotes3.clear();

    static QUrl url("https://chatterinohomies.com/api/badges/list");

    NetworkRequest(url)
        .concurrent()
        .onSuccess([this](auto result) -> Outcome {
            auto jsonRoot = result.parseJson();

            std::unique_lock lock(this->mutex_);

            int index = 0;
            for (const auto &jsonBadge_ : jsonRoot.value("badges").toArray())
            {
                auto jsonBadge = jsonBadge_.toObject();
                auto emote = Emote{
                    EmoteName{},
                    ImageSet{Url{jsonBadge.value("image1").toString()},
                             Url{jsonBadge.value("image2").toString()},
                             Url{jsonBadge.value("image3").toString()}},
                    Tooltip{jsonBadge.value("tooltip").toString()}, Url{}};

                emotes.push_back(
                    std::make_shared<const Emote>(std::move(emote)));

                badgeMap[jsonBadge.value("userId").toString()] = index;
                ++index;
            }
            return Success;
        })
        .execute();

    static QUrl url2("https://itzalex.github.io/badges");

    NetworkRequest(url2)
        .concurrent()
        .onSuccess([this](auto result) -> Outcome {
            auto jsonRoot = result.parseJson();

            std::unique_lock lock(this->mutex_);

            int index = 0;
            for (const auto &jsonBadge_ : jsonRoot.value("badges").toArray())
            {
                auto jsonBadge = jsonBadge_.toObject();
                auto emote = Emote{
                    EmoteName{},
                    ImageSet{Url{jsonBadge.value("image1").toString()},
                             Url{jsonBadge.value("image2").toString()},
                             Url{jsonBadge.value("image3").toString()}},
                    Tooltip{jsonBadge.value("tooltip").toString()}, Url{}};

                emotes2.push_back(
                    std::make_shared<const Emote>(std::move(emote)));

                for (const auto &user : jsonBadge.value("users").toArray())
                {
                    badgeMap2[user.toString()] = index;
                }
                ++index;
            }
            return Success;
        })
        .execute();

    static QUrl url3("https://itzalex.github.io/badges2");

    NetworkRequest(url3)
        .concurrent()
        .onSuccess([this](auto result) -> Outcome {
            auto jsonRoot = result.parseJson();

            std::unique_lock lock(this->mutex_);

            int index = 0;
            for (const auto &jsonBadge_ : jsonRoot.value("badges").toArray())
            {
                auto jsonBadge = jsonBadge_.toObject();
                auto emote = Emote{
                    EmoteName{},
                    ImageSet{Url{jsonBadge.value("image1").toString()},
                             Url{jsonBadge.value("image2").toString()},
                             Url{jsonBadge.value("image3").toString()}},
                    Tooltip{jsonBadge.value("tooltip").toString()}, Url{}};

                emotes3.push_back(
                    std::make_shared<const Emote>(std::move(emote)));

                for (const auto &user : jsonBadge.value("users").toArray())
                {
                    badgeMap3[user.toString()] = index;
                }
                ++index;
            }
            return Success;
        })
        .execute();
}
}  // namespace chatterino