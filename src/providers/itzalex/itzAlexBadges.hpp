#pragma once

#include <boost/optional.hpp>
#include <common/Singleton.hpp>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include "common/Aliases.hpp"
#include "util/QStringHash.hpp"

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class itzAlexBadges : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;
    itzAlexBadges();

    boost::optional<EmotePtr> getBadge(const UserId &id);

private:
    void loaditzAlexBadges();

    std::shared_mutex mutex_;

    std::unordered_map<QString, int> badgeMap;
    std::vector<EmotePtr> emotes;
};

}  // namespace chatterino
