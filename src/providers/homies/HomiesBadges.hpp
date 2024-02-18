#pragma once

#include "common/Aliases.hpp"
#include "common/Singleton.hpp"
#include "util/QStringHash.hpp"

#include <boost/optional.hpp>

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class IHomiesBadges
{
public:
    IHomiesBadges() = default;
    virtual ~IHomiesBadges() = default;

    IHomiesBadges(const IHomiesBadges &) = delete;
    IHomiesBadges(IHomiesBadges &&) = delete;
    IHomiesBadges &operator=(const IHomiesBadges &) = delete;
    IHomiesBadges &operator=(IHomiesBadges &&) = delete;

    virtual boost::optional<EmotePtr> getBadge(const UserId &id) = 0;
    virtual boost::optional<EmotePtr> getBadge2(const UserId &id) = 0;
    virtual boost::optional<EmotePtr> getBadge3(const UserId &id) = 0;
};

class HomiesBadges : public IHomiesBadges
{
public:
    HomiesBadges();

    boost::optional<EmotePtr> getBadge(const UserId &id) override;
    boost::optional<EmotePtr> getBadge2(const UserId &id) override;
    boost::optional<EmotePtr> getBadge3(const UserId &id) override;

private:
    void loadHomiesBadges();
    std::shared_mutex mutex_;

    std::unordered_map<QString, int> badgeMap;
    std::unordered_map<QString, int> badgeMap2;
    std::unordered_map<QString, int> badgeMap3;
    std::vector<EmotePtr> emotes;
    std::vector<EmotePtr> emotes2;
    std::vector<EmotePtr> emotes3;
};

}  // namespace chatterino