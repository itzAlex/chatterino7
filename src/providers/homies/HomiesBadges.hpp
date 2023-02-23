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

class HomiesBadges : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;
    HomiesBadges();
    void loadHomiesBadges();

    boost::optional<EmotePtr> getBadge(const UserId &id);
    boost::optional<EmotePtr> getBadge2(const UserId &id);
    boost::optional<EmotePtr> getBadge3(const UserId &id);

private:
    std::shared_mutex mutex_;

    std::unordered_map<QString, int> badgeMap;
    std::unordered_map<QString, int> badgeMap2;
    std::unordered_map<QString, int> badgeMap3;
    std::vector<EmotePtr> emotes;
    std::vector<EmotePtr> emotes2;
    std::vector<EmotePtr> emotes3;
};

}  // namespace chatterino