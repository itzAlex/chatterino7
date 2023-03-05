#include "providers/twitch/api/GraphQL.hpp"

namespace chatterino {
static IGraphQL *instance = nullptr;

void GraphQL::followUser(QString targetId,
                         std::function<void()> successCallback,
                         GraphQLFailureCallback failureCallback)
{
    QJsonObject payload, variables, input, extensions, persistedQuery;

    input.insert("disableNotifications", true);  // disable notifications
    input.insert("targetID", targetId);          // channel ID to follow

    variables.insert("input", input);

    // operation hash
    persistedQuery.insert(
        "sha256Hash",
        "800e7346bdf7e5278a3c1d3f21b2b56e2639928f86815677a7126b093b2fdd08");
    persistedQuery.insert("version", 1);

    extensions.insert("persistedQuery", persistedQuery);

    // operation name
    payload.insert("operationName", "FollowButton_FollowUser");
    payload.insert("variables", variables);
    payload.insert("extensions", extensions);

    this->makeRequest(payload)
        .onSuccess([successCallback](auto /*result*/) -> Outcome {
            successCallback();
            return Success;
        })
        .onError([failureCallback](auto /*result*/) {
            failureCallback();
        })
        .execute();
}

void GraphQL::unfollowUser(QString targetId,
                           std::function<void()> successCallback,
                           GraphQLFailureCallback failureCallback)
{
    QJsonObject payload, variables, input, extensions, persistedQuery;

    input.insert("targetID", targetId);  // channel ID to follow

    variables.insert("input", input);

    // operation hash
    persistedQuery.insert(
        "sha256Hash",
        "f7dae976ebf41c755ae2d758546bfd176b4eeb856656098bb40e0a672ca0d880");
    persistedQuery.insert("version", 1);

    extensions.insert("persistedQuery", persistedQuery);

    // operation name
    payload.insert("operationName", "FollowButton_UnfollowUser");
    payload.insert("variables", variables);
    payload.insert("extensions", extensions);

    this->makeRequest(payload)
        .onSuccess([successCallback](auto /*result*/) -> Outcome {
            successCallback();
            return Success;
        })
        .onError([failureCallback](auto /*result*/) {
            failureCallback();
        })
        .execute();
}

NetworkRequest GraphQL::makeRequest(QJsonObject payload)
{
    if (this->clientId.isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "GraphQL::makeRequest called without a client ID set BabyRage";
        // return boost::none;
    }

    if (this->oauthToken.isEmpty())
    {
        qCDebug(chatterinoTwitch) << "GraphQL::makeRequest called without an "
                                     "oauth token set BabyRage";
        // return boost::none;
    }

    const QString GQLUrl("https://gql.twitch.tv/gql");

    return NetworkRequest(GQLUrl, NetworkRequestType::Post)
        .timeout(5 * 1000)
        .header("Content-Type", "application/json")
        .header("Authorization", "OAuth " + this->oauthToken)
        .payload(QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

void GraphQL::update(QString clientId, QString oauthToken)
{
    this->clientId = std::move(clientId);
    this->oauthToken = std::move(oauthToken);
}

void GraphQL::initialize()
{
    assert(instance == nullptr);

    initializeGraphQL(new GraphQL());
}

void initializeGraphQL(IGraphQL *_instance)
{
    assert(_instance != nullptr);

    instance = _instance;
}

IGraphQL *getGraphQL()
{
    assert(instance != nullptr);

    return instance;
}
}  // namespace chatterino