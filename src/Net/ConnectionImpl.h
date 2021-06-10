#include "INetEvent.h"

class CConnectionImpl final : public ec_net::IConnection
{
  enum class EConnectState
  {
    eCONNECT_STATE_NONE = 0,
    eCONNECT_STATE_CONNECTING = 1,
    eCONNECT_STATE_OK = 2,
    eCONNECT_STATE_CLOSING = 3,
  };
public:
  explicit CConnectionImpl(SocketId id);
  ~CConnectionImpl();

  void connect(const char* ip, Port port, BuffSize recv, BuffSize send) override;
  void disconnect() override;
  void sendProtoMsg(const char* msgFullName, const void* pData, size_t size) override;
  void setNetEvent(ec_net::INetEvent* event) override;
  SocketId getId() override;
  bool isConnected() override;

  void onConnectSucceed(const ec_net::QNetConnectSucceedEvent& event);
  void onDisconnect(const ec_net::QNetDisconnectEvent& event);
  void onError(const ec_net::QNetErrorEvent& event);
  void onParseMessage(const ec_net::QNetParseMessageEvent& event);

private:
  ec_net::INetEvent* m_event = nullptr;
  SocketId m_id;
  EConnectState m_state;
};
