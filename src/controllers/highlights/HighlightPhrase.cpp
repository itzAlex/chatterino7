#include "controllers/highlights/HighlightPhrase.hpp"

namespace chatterino {

namespace {

    const QString REGEX_START_BOUNDARY("(\\b|\\s|^)");
    const QString REGEX_END_BOUNDARY("(\\b|\\s|$)");

}  // namespace

QColor HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR = QColor(127, 63, 73, 127);
QColor HighlightPhrase::FALLBACK_REDEEMED_HIGHLIGHT_COLOR =
    QColor(28, 126, 141, 60);
QColor HighlightPhrase::FALLBACK_FIRST_MESSAGE_HIGHLIGHT_COLOR =
    QColor(72, 127, 63, 60);
QColor HighlightPhrase::FALLBACK_SUB_COLOR = QColor(196, 102, 255, 100);

bool HighlightPhrase::operator==(const HighlightPhrase &other) const
{
    return std::tie(this->pattern_, this->showInMentions_, this->hasSound_,
                    this->hasAlert_, this->isRegex_, this->isCaseSensitive_,
                    this->soundUrl_, this->color_, this->globalHighlight_,
                    this->channels_, this->ExcludedChannels_) ==
           std::tie(other.pattern_, other.showInMentions_, other.hasSound_,
                    other.hasAlert_, other.isRegex_, other.isCaseSensitive_,
                    other.soundUrl_, other.color_, other.globalHighlight_,
                    other.channels_, other.ExcludedChannels_);
}

HighlightPhrase::HighlightPhrase(const QString &pattern, bool showInMentions,
                                 bool hasAlert, bool hasSound, bool isRegex,
                                 bool isCaseSensitive, const QString &soundUrl,
                                 QColor color, bool globalHighlight,
                                 std::vector<std::string> channels,
                                 std::vector<std::string> ExcludedChannels)
    : pattern_(pattern)
    , showInMentions_(showInMentions)
    , hasAlert_(hasAlert)
    , hasSound_(hasSound)
    , isRegex_(isRegex)
    , isCaseSensitive_(isCaseSensitive)
    , soundUrl_(soundUrl)
    , regex_(isRegex_
                 ? pattern
                 : REGEX_START_BOUNDARY + QRegularExpression::escape(pattern) +
                       REGEX_END_BOUNDARY,
             QRegularExpression::UseUnicodePropertiesOption |
                 (isCaseSensitive_ ? QRegularExpression::NoPatternOption
                                   : QRegularExpression::CaseInsensitiveOption))
    , globalHighlight_(globalHighlight)
    , channels_(channels)
    , ExcludedChannels_(ExcludedChannels)
{
    this->color_ = std::make_shared<QColor>(color);
}

HighlightPhrase::HighlightPhrase(const QString &pattern, bool showInMentions,
                                 bool hasAlert, bool hasSound, bool isRegex,
                                 bool isCaseSensitive, const QString &soundUrl,
                                 std::shared_ptr<QColor> color,
                                 bool globalHighlight,
                                 std::vector<std::string> channels,
                                 std::vector<std::string> ExcludedChannels)
    : pattern_(pattern)
    , showInMentions_(showInMentions)
    , hasAlert_(hasAlert)
    , hasSound_(hasSound)
    , isRegex_(isRegex)
    , isCaseSensitive_(isCaseSensitive)
    , soundUrl_(soundUrl)
    , color_(std::move(color))
    , regex_(isRegex_
                 ? pattern
                 : REGEX_START_BOUNDARY + QRegularExpression::escape(pattern) +
                       REGEX_END_BOUNDARY,
             QRegularExpression::UseUnicodePropertiesOption |
                 (isCaseSensitive_ ? QRegularExpression::NoPatternOption
                                   : QRegularExpression::CaseInsensitiveOption))
    , globalHighlight_(globalHighlight)
    , channels_(channels)
    , ExcludedChannels_(ExcludedChannels)
{
}

const QString &HighlightPhrase::getPattern() const
{
    return this->pattern_;
}

bool HighlightPhrase::showInMentions() const
{
    return this->showInMentions_;
}

bool HighlightPhrase::hasAlert() const
{
    return this->hasAlert_;
}

bool HighlightPhrase::hasSound() const
{
    return this->hasSound_;
}

bool HighlightPhrase::hasCustomSound() const
{
    return !this->soundUrl_.isEmpty();
}

bool HighlightPhrase::isRegex() const
{
    return this->isRegex_;
}

bool HighlightPhrase::isValid() const
{
    return !this->pattern_.isEmpty() && this->regex_.isValid();
}

bool HighlightPhrase::isMatch(const QString &subject) const
{
    return this->isValid() && this->regex_.match(subject).hasMatch();
}

bool HighlightPhrase::isCaseSensitive() const
{
    return this->isCaseSensitive_;
}

const QUrl &HighlightPhrase::getSoundUrl() const
{
    return this->soundUrl_;
}

const std::shared_ptr<QColor> HighlightPhrase::getColor() const
{
    return this->color_;
}

bool HighlightPhrase::isGlobalHighlight() const
{
    return this->globalHighlight_;
}

const std::vector<std::string> &HighlightPhrase::getChannels() const
{
    return this->channels_;
}

const std::vector<std::string> &HighlightPhrase::getExcludedChannels() const
{
    return this->ExcludedChannels_;
}

}  // namespace chatterino
