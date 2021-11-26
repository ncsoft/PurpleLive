#pragma once

#include <QtCore/qglobal.h>
#include <map>
#include <string>
#include <vector>

using namespace std;

class StompMessage
{
public:
    /* This constructor is not meant for the user, just for the dll*/
    StompMessage(const char* rawMessage);

    StompMessage(string messageType, map<string, string> headers, const char* messageBody = "");

    /* Creates a string with the message in a readable format =)*/
    std::string toString() const;

    enum MessageType { CONNECT, SUBSCRIBE, SEND, MESSAGE };

    map<string, string> m_headers;
    string m_message;
    string m_messageType;

private:

    vector<string> messageToVector(const string& str, const string& delim);
};


