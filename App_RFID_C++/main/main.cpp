#include <QCoreApplication>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QObject>
#include <QList>

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    // Configuration du port s�rie
    QSerialPort serialPort;
    serialPort.setPortName("COM4"); // Remplacez "COMx" par le port de votre Arduino
    serialPort.setBaudRate(QSerialPort::Baud9600); // Assurez-vous que la vitesse correspond � celle de votre Arduino

    // Ouverture du port s�rie
    if (!serialPort.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open serial port";
        return 1;
    }

    QWebSocketServer server(QStringLiteral("WebSocket Server"), QWebSocketServer::NonSecureMode);

    if (!server.listen(QHostAddress::Any, 12345)) {
        qCritical() << "Failed to start WebSocket server:" << server.errorString();
        return 1;
    }

    qDebug() << "WebSocket server listening on port" << server.serverPort();

    // Liste pour stocker les sockets des clients WebSocket
    QList<QWebSocket*> clients;

    // Fonction de rappel appel�e lorsqu'une nouvelle connexion WebSocket est �tablie
    QObject::connect(&server, &QWebSocketServer::newConnection, [&]() {
        // R�cup�rez le socket WebSocket pour cette connexion
        QWebSocket* webSocket = server.nextPendingConnection();

        // Ajoutez le nouveau client � la liste
        clients.append(webSocket);
        
        // Fonction de rappel appel�e lorsqu'une nouvelle donn�e est re�ue du client WebSocket
        QObject::connect(webSocket, &QWebSocket::textMessageReceived, [&](const QString& message) {
            qDebug() << "Received data from WebSocket:" << message;
            // Ajoutez ici votre logique pour traiter les donn�es WebSocket comme vous le souhaitez
            });

        // Fonction de rappel appel�e lorsqu'un client WebSocket est d�connect�
        QObject::connect(webSocket, &QWebSocket::disconnected, [&]() {
            qDebug() << "WebSocket client disconnected";
            // Supprimez le client d�connect� de la liste
            clients.removeOne(webSocket);
            webSocket->deleteLater(); // Lib�rez les ressources lorsque le client est d�connect�
            });
        });

    // Lecture et affichage des donn�es en continu du port s�rie
    QByteArray receivedData;  // Variable pour stocker les donn�es re�ues

    while (true) {
        if (serialPort.waitForReadyRead(100)) {
            receivedData.append(serialPort.readAll());

            // V�rifiez s'il y a un caract�re de fin (par exemple, '\n') pour afficher les donn�es sur une seule ligne
            if (receivedData.contains('\n')) {
                // Retirez le caract�re de fin et les caract�res de retour chariot
                receivedData = receivedData.replace("\n", "").replace("\r", "");

                // Supprimez tous les espaces
                receivedData.replace(" ", "");

                qDebug() << "Received data from Serial Port:" << receivedData;

                // Envoyez les donn�es � tous les clients WebSocket connect�s
                for (QWebSocket* client : clients) {
                    client->sendTextMessage(receivedData);
                }

                // R�initialisez la variable pour la prochaine ligne de donn�es
                receivedData.clear();
            }
        }
    }

    // Retournez 0 � la fin de la fonction main
    return 0;
}
