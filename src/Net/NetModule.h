#include <thread>
#include <asio/io_context.hpp>
#include <QApplication>

#include "INetEvent.h"
#include "SessionManager.h"
#include "Singleton.h"

class CConnectionImpl;
class CNetModule : public QObject
{
public:
  CNetModule() : m_sessionMgr(m_ioContext) {}
  ~CNetModule() = default;

  bool start();
  void stop();

  template <typename COMPLETE_HANDLER>
  void postToNet(COMPLETE_HANDLER&& handler);
  void connect(SocketId socketId, const char* ip, Port port, BuffSize recv, BuffSize send);
  void disconnect(SocketId socketId);
  void sendProtoMsg(SocketId socketId, std::string msgFullName, const void* pData, size_t size);

  void postToApp(QEvent* event, int priority = Qt::NormalEventPriority);
  bool event(QEvent* event) override;
  void handleConnectSucceed(SocketId socketId, const std::string& ip, Port port);
  void handleDisconnect(SocketId socketId);
  void handleError(SocketId socketId, ec_net::ENetError error);
  void handleParseMessage(SocketId socketId, MessageType msgType, const char* fullName, const char* pData, size_t size);

  ec_net::IConnection* createConnection();
  void releaseConnection(ec_net::IConnection* connection);

private:
  asio::io_context m_ioContext;
  std::unique_ptr<std::thread> m_asioThead;
  std::unordered_map<int32_t, CConnectionImpl*> m_mapConnections;
  CSessionManager m_sessionMgr;
  SocketId m_increaseId = 0;
};

template <typename COMPLETE_HANDLER>
void CNetModule::postToNet(COMPLETE_HANDLER&& handler) {
  m_ioContext.post(std::move(handler));
}

typedef CSingleton<CNetModule> NetModule;
