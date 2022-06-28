#include "Chat.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void ChatMessage::to_bin()
{
    alloc_data(MESSAGE_SIZE);

    memset(_data, 0, MESSAGE_SIZE);

    //Serializar los campos type, nick y message en el buffer _data
    char *tmp = _data;
    memcpy(tmp, &type, sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(tmp, nick.c_str(), sizeof(char) * 8);
    tmp += sizeof(char) * 8;
    memcpy(tmp, message.c_str(), sizeof(char) * 80);
}

int ChatMessage::from_bin(char * bobj)
{
    alloc_data(MESSAGE_SIZE);

    memcpy(static_cast<void *>(_data), bobj, MESSAGE_SIZE);

    //Reconstruir la clase usando el buffer _data
    char *tmp = _data;
    memcpy(&type, tmp, sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    nick = tmp;
    tmp += sizeof(char) * 8;
    message = tmp;

    return 0;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void ChatServer::do_messages()
{
    while (true)
    {
        ChatMessage chatMsg;
        Socket *s;

        //Recibir un mensaje de cualquier socket
        socket.recv(chatMsg, s);

        /*
         * NOTA: los clientes están definidos con "smart pointers", es necesario
         * crear un unique_ptr con el objeto socket recibido y usar std::move
         * para añadirlo al vector
         */

        //Recibir Mensajes en y en función del tipo de mensaje
        // - LOGIN: Añadir al vector clients
        // - LOGOUT: Eliminar del vector clients
        // - MESSAGE: Reenviar el mensaje a todos los clientes (menos el emisor)

        switch (chatMsg.type)
        {
            case ChatMessage::LOGIN:
            {         
                std::cout << "User " << chatMsg.nick << " conectado\n";
                clients.push_back(std::move(std::make_unique<Socket>(*s)));
                break;
            }

            case ChatMessage::LOGOUT:
            {           
                auto it = clients.begin();

                while (it != clients.end() && (**it != *s))
                    ++it;

                if (it == clients.end())
                    std::cout << "User " << chatMsg.nick << " no se encontraba conectado\n";
                else
                {
                    std::cout << "User " << chatMsg.nick << " desconectado\n";
                    clients.erase(it);          
                }
                break;
            }

            case ChatMessage::MESSAGE:
            {            
                for (auto it = clients.begin(); it != clients.end(); ++it)
                {
                    if (**it != *s)				
                        if(socket.send(chatMsg, **it) == -1)
                        {
                            std::cout << "User " << chatMsg.nick << " desconectado\n";
                            clients.erase(it);
                        }		
                }
                break;
            }

            default:
                std::cerr << "ERROR tipo de mensaje no reconocido\n";
                break;
            }              
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void ChatClient::login()
{
    std::string msg;

    ChatMessage cm(nick, msg);
    cm.type = ChatMessage::LOGIN;

    socket.send(cm, socket);
    
}

void ChatClient::logout()
{
    std::string msg;

    ChatMessage cm(nick, msg);
    cm.type = ChatMessage::LOGOUT;

    socket.send(cm, socket);
}

void ChatClient::input_thread() {
    std::string msg;

    do {
        std::getline(std::cin, msg);

        if (msg.size() > 80) msg.resize(80); //80 Tamaño máximo predefinido en la clase

        ChatMessage cm(nick, msg);
        cm.type = ChatMessage::MESSAGE;
        socket.send(cm, socket);
    } while (msg != "exit");

    logout();
}

void ChatClient::net_thread()
{
    while (true) {
        ChatMessage cm;

        socket.recv(cm);
        std::cout << cm.nick << ": " << cm.message << "\n";
    }
}
