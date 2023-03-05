#pragma once

#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"

#include <QJsonObject>
#include <QString>

namespace chatterino {
using GraphQLFailureCallback = std::function<void()>;
template <typename... T>
using ResultCallback = std::function<void(T...)>;

class IGraphQL
{
public:
    virtual void followUser(QString targetId,
                            std::function<void()> successCallback,
                            GraphQLFailureCallback failureCallback) = 0;
    virtual void unfollowUser(QString targetId,
                              std::function<void()> successCallback,
                              GraphQLFailureCallback failureCallback) = 0;
    virtual void update(QString clientId, QString oauthToken) = 0;
};

class GraphQL final : public IGraphQL
{
public:
    void followUser(QString targetId, std::function<void()> successCallback,
                    GraphQLFailureCallback failureCallback) final;
    void unfollowUser(QString targetId, std::function<void()> successCallback,
                      GraphQLFailureCallback failureCallback) final;
    void update(QString clientId, QString oauthToken) final;

    static void initialize();

private:
    NetworkRequest makeRequest(QJsonObject payload);

    QString clientId;
    QString oauthToken;
};

void initializeGraphQL(IGraphQL *_instance);
IGraphQL *getGraphQL();
}  // namespace chatterino