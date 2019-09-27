/*
  WebServer.cpp - Lightweight HTTP Webserver.
  Copyright 2019, SytheZN, All rights reserved.
*/
#include "WebServer.h"

WebServer::WebServer(WiFiServer *server)
    : _api_GETs(NULL), _api_GETsLength(0), _api_PUTs(NULL), _api_PUTsLength(0), _api_POSTs(NULL), _api_POSTsLength(0), _api_DELETEs(NULL), _api_DELETEsLength(0)
{
  _server = server;
}

bool WebServer::handle()
{
  if (_processStep != ProcessStep::AwaitClient || _errorState != ErrorState::None || _client)
    loop();
  else
    _client = _server->available();

  return _client ? true : false;
}

void WebServer::SetGetHandlers(ApiMethod *apiMethods, uint8_t count)
{
  for (uint8_t i = 0; i < count; i++)
  {
    apiMethods[i].Path = "/api/" + apiMethods[i].Path;
    apiMethods[i].Path.toLowerCase();
  }
  _api_GETs = apiMethods;
  _api_GETsLength = count;
}
void WebServer::SetPutHandlers(ApiMethod *apiMethods, uint8_t count)
{
  for (uint8_t i = 0; i < count; i++)
  {
    apiMethods[i].Path = "/api/" + apiMethods[i].Path;
    apiMethods[i].Path.toLowerCase();
  }
  _api_PUTs = apiMethods;
  _api_PUTsLength = count;
}
void WebServer::SetPostHandlers(ApiMethod *apiMethods, uint8_t count)
{
  for (uint8_t i = 0; i < count; i++)
  {
    apiMethods[i].Path = "/api/" + apiMethods[i].Path;
    apiMethods[i].Path.toLowerCase();
  }
  _api_POSTs = apiMethods;
  _api_POSTsLength = count;
}
void WebServer::SetDeleteHandlers(ApiMethod *apiMethods, uint8_t count)
{
  for (uint8_t i = 0; i < count; i++)
  {
    apiMethods[i].Path = "/api/" + apiMethods[i].Path;
    apiMethods[i].Path.toLowerCase();
  }
  _api_DELETEs = apiMethods;
  _api_DELETEsLength = count;
}

void WebServer::loop()
{
  if (_errorState != ErrorState::None)
  {
    handleError();
    return;
  }

  _processStep = (ProcessStep)(((uint8_t)_processStep) + 1);
  switch (_processStep)
  {
  case ProcessStep::GetRequestHeader:
    Serial.print("Client connected: ");
    Serial.println(_client.remoteIP().toString());
    readClientRequestHeader();
    break;

  case ProcessStep::ParseRequestHeader:
    Serial.print("Got request: ");
    Serial.println(_requestHeader);
    parseRequestHeader();
    break;

  case ProcessStep::SelectRequestMethodHandler:
    Serial.println("Request Parameters:");
    Serial.print("  Method: ");
    Serial.println(_requestHeaderParts[0]);
    Serial.print("  Path:   ");
    Serial.println(_requestHeaderParts[1]);
    Serial.print("  Proto:  ");
    Serial.println(_requestHeaderParts[2]);
    selectRequestMethod();
    break;

  case ProcessStep::ProcessRequest_GET:
    Serial.println("Enter GET");
    selectRequestPath_GET();
    break;
  case ProcessStep::ProcessRequest_POST:
    Serial.println("Enter POST");
    selectRequestPath_POST();
    break;
  case ProcessStep::ProcessRequest_PUT:
    Serial.println("Enter PUT");
    selectRequestPath_PUT();
    break;
  case ProcessStep::ProcessRequest_DELETE:
    Serial.println("Enter DELETE");
    selectRequestPath_DELETE();
    break;

  case ProcessStep::EndRequest_GET:
  case ProcessStep::EndRequest_POST:
  case ProcessStep::EndRequest_PUT:
  case ProcessStep::EndRequest_DELETE:
    Serial.println("Graceful disconnect.\n");
    _client.stop();
    resetState();
    return;

  default:
    // something went wrong, but the _client is still connected.
    Serial.printf("processStep out of bounds: %d\r\nDisconnecting _client.\n\r\n", _processStep);
    _client.stop();
    resetState();
    return;
  }
}

bool WebServer::isFileNameLegal(String path)
{
  const char legalChars[] = "0123456789abcdefghijklmnopqrstuvwxyz.";

  bool isLegal = true;
  bool periodFound = false;

  for (int pathIndex = 0; pathIndex < path.length(); pathIndex++)
  {
    bool isLegalChar = false;
    for (uint8_t legalIndex = 0; legalIndex < 38; legalIndex++)
    {
      if (path[pathIndex] == legalChars[legalIndex])
        isLegalChar = true;
    }

    if (path[pathIndex] == legalChars[37])
    {
      if (!periodFound)
        periodFound = true;
      else
        isLegal = false;
    }

    if (!isLegalChar)
      isLegal = false;
  }

  return isLegal;
}
void WebServer::clearClientBuffer()
{
  while (_client.connected())
  {
    if (_client.available())
      _client.read();
    else
      return;
  }
}

void WebServer::handleError()
{
  if (_errorHandled)
  {
    if (_client.connected())
    {
      Serial.println("Disconnecting client.\n");
      _client.stop();
    }
    resetState();
  }
  else
  {
    switch (_errorState)
    {
    case ErrorState::ReadTimeout:
      Serial.println("Read timeout");
      break;

    case ErrorState::BadRequest:
      writeError("400 Bad Request");
      Serial.println("Returned 400 Bad Request");
      break;

    case ErrorState::MethodNotAllowed:
      writeError("405 Method Not Allowed");
      Serial.println("Returned 405 Method Not Allowed");
      break;

    case ErrorState::NotAcceptable:
      writeError("406 Not Acceptable");
      Serial.println("Returned 406 Not Acceptable");
      break;

    case ErrorState::Conflict:
      writeError("409 Conflict");
      Serial.println("Returned 409 Conflict");
      break;

    case ErrorState::NotFound:
      writeError("404 Not Found");
      Serial.println("Returned 404 Not Found");
      break;

    case ErrorState::InternalServerError:
      writeError("500 Internal Server Error");
      Serial.println("Returned 500 Internal Server Error");
      break;

    default:
      Serial.printf("errorState out of bounds: %d\r\n");
    }
    _errorHandled = true;
  }
}
void WebServer::writeError(const String response)
{
  clearClientBuffer();
  _client.println("HTTP/1.1 " + response + "\r\nConnection: Closed\r\n");
}
void WebServer::resetState()
{
  _requestHeader = "";
  _requestHeaderParts[0] = "";
  _requestHeaderParts[1] = "";
  _requestHeaderParts[2] = "";
  _processStep = ProcessStep::AwaitClient;
  _errorState = ErrorState::None;
  _errorHandled = false;
}

void WebServer::readClientRequestHeader()
{
  long readtimeout = millis() + READ_TIMEOUT;
  while (_client.connected())
  {
    if (millis() > readtimeout)
    {
      _errorState = ErrorState::ReadTimeout;
      break;
    }

    if (_client.available())
    {
      if (_requestHeader == "")
        _requestHeader = _client.readStringUntil('\r'); // get the _requestHeader string
      else
      {
        String line = _client.readStringUntil('\r'); //swallow headers until we hit the _requestHeader header break
        if (line.length() == 1 && line[0] == '\n')
        {
          _client.read();
          break;
        }
      }
    }
    else
      yield();
  }
}

void WebServer::parseRequestHeader()
{
  uint requestLen = _requestHeader.length() + 1;
  char c_request[requestLen];
  _requestHeader.toCharArray(c_request, requestLen);

  uint8_t index = 0;
  const char delimiter[2] = " ";
  char *substring;

  substring = strtok(c_request, delimiter);
  while (substring != NULL)
  {
    if (index < 3)
    {
      _requestHeaderParts[index] = (String)substring;
    }
    substring = strtok(NULL, delimiter);
    index++;
  }

  if (index != 3)
    _errorState = ErrorState::BadRequest;
  else
    _requestHeaderParts[1].toLowerCase();
}

void WebServer::selectRequestMethod()
{
  if (_requestHeaderParts[0] == "GET")
  {
    _processStep = ProcessStep::RequestMethod_GET;
  }
  else if (_requestHeaderParts[0] == "POST")
  {
    _processStep = ProcessStep::RequestMethod_POST;
  }
  else if (_requestHeaderParts[0] == "PUT")
  {
    _processStep = ProcessStep::RequestMethod_PUT;
  }
  else if (_requestHeaderParts[0] == "DELETE")
  {
    _processStep = ProcessStep::RequestMethod_DELETE;
  }
  else
  {
    _errorState = ErrorState::MethodNotAllowed;
  }
}

void WebServer::selectRequestPath_GET()
{
  if (_requestHeaderParts[1] == "/filelist")
  {
    serve_GET_fileList();
    return;
  }

  if (_requestHeaderParts[1].startsWith("/file/"))
  {
    serve_GET_file();
    return;
  }

  for (uint8_t i = 0; i < _api_GETsLength; i++)
  {
    Serial.println("Compare selector: '" + _api_GETs[i].Path + "'");
    if (_requestHeaderParts[1] == _api_GETs[i].Path)
    {
      Serial.println("  Match");
      serve_api(_api_GETs[i].Callback);
      return;
    }
  }

  _errorState = ErrorState::NotFound;
  return;
}
void WebServer::selectRequestPath_PUT()
{
  if (_requestHeaderParts[1].startsWith("/file/"))
  {
    serve_PUT_file();
    return;
  }

  for (uint8_t i = 0; i < _api_PUTsLength; i++)
  {
    Serial.println("Compare selector: '" + _api_PUTs[i].Path + "'");
    if (_requestHeaderParts[1] == _api_PUTs[i].Path)
    {
      Serial.println("  Match");
      serve_api(_api_PUTs[i].Callback);
      return;
    }
  }

  _errorState = ErrorState::NotFound;
  return;
}
void WebServer::selectRequestPath_POST()
{
  for (uint8_t i = 0; i < _api_POSTsLength; i++)
  {
    Serial.println("Compare selector: '" + _api_POSTs[i].Path + "'");
    if (_requestHeaderParts[1] == _api_POSTs[i].Path)
    {
      Serial.println("  Match");
      serve_api(_api_POSTs[i].Callback);
      return;
    }
  }

  _errorState = ErrorState::NotFound;
  return;
}
void WebServer::selectRequestPath_DELETE()
{
  if (_requestHeaderParts[1].startsWith("/file/"))
  {
    serve_DELETE_file();
    return;
  }

  for (uint8_t i = 0; i < _api_DELETEsLength; i++)
  {
    Serial.println("Compare selector: '" + _api_DELETEs[i].Path + "'");
    if (_requestHeaderParts[1] == _api_DELETEs[i].Path)
    {
      Serial.println("  Match");
      serve_api(_api_DELETEs[i].Callback);
      return;
    }
  }

  _errorState = ErrorState::NotFound;
  return;
}

void WebServer::serve_GET_fileList()
{
  FSInfo info;
  SPIFFS.info(info);
  String fileList = "";
  fileList += String(info.totalBytes - info.usedBytes);
  fileList += " bytes available of ";
  fileList += String(info.totalBytes);
  auto dir = SPIFFS.openDir("");
  fileList += "\r\n\r\nFiles:\r\n";

  while (dir.next())
  {
    fileList += "    ";
    fileList += dir.fileName();
    fileList += "    ";
    fileList += String(dir.fileSize());
    fileList += "\r\n";
  }
  clearClientBuffer();
  _client.printf("HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/plain\r\n\r\n", fileList.length());
  _client.print(fileList);
}
void WebServer::serve_GET_file()
{
  String path = _requestHeaderParts[1];
  path.remove(0, 6);

  if (!isFileNameLegal(path))
  {
    _errorState = ErrorState::NotAcceptable;
    return;
  }

  if (!SPIFFS.exists(path))
  {
    _errorState = ErrorState::NotFound;
    return;
  }

  auto f = SPIFFS.open(path, "r");
  if (!f)
  {
    Serial.println("FS: Failed to open '" + path + "'");
    _errorState = ErrorState::InternalServerError;
    return;
  }

  clearClientBuffer();
  _client.printf("HTTP/1.1 200 OK\r\nContent-Length: %d\r\n", f.size());

  if (path.endsWith(".html") || path.endsWith(".htm"))
    _client.println("Content-Type: text/html");
  if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
    _client.println("Content-Type: image/jpeg");
  if (path.endsWith(".png"))
    _client.println("Content-Type: image/png");
  if (path.endsWith(".js"))
    _client.println("Content-Type: application/javascript");

  _client.println();

  int remaining = f.size();
  char buffer[WEBSERVER_FILE_BUFFER_LENGTH];

  while (remaining)
  {
    uint8_t chunk = remaining > WEBSERVER_FILE_BUFFER_LENGTH ? WEBSERVER_FILE_BUFFER_LENGTH : remaining;
    f.readBytes(buffer, chunk);
    _client.write_P(buffer, chunk);
    remaining -= chunk;
  }
}
void WebServer::serve_PUT_file()
{
  String path = _requestHeaderParts[1];
  path.remove(0, 6);

  if (!isFileNameLegal(path))
  {
    _errorState = ErrorState::NotAcceptable;
    return;
  }

  auto f = SPIFFS.open(path, "w");
  if (!f)
  {
    Serial.println("FS: Failed to open '" + path + "'");
    _errorState = ErrorState::InternalServerError;
    return;
  }
  if (f.size() != 0)
  {
    f.close();
    Serial.println("FS: file already contains data '" + path + "'");
    _errorState = ErrorState::Conflict;
    return;
  }

  while (_client.available())
  {
    byte b = _client.read();
    f.write(b);
    delay(0);
  }
  f.flush();
  f.close();

  _client.println("HTTP/1.1 200 OK\r\nConnection: Closed\r\n");
}
void WebServer::serve_DELETE_file()
{
  String path = _requestHeaderParts[1];
  path.remove(0, 6);

  if (!isFileNameLegal(path))
  {
    _errorState = ErrorState::NotAcceptable;
    return;
  }

  if (!SPIFFS.exists(path))
  {
    _errorState = ErrorState::NotFound;
    return;
  }

  SPIFFS.remove(path);

  clearClientBuffer();
  _client.println("HTTP/1.1 200 OK\r\nConnection: Closed\r\n");
}

void WebServer::serve_api(ApiMethod::CallbackFunction fn)
{
  ApiMethodResponse response;
  response.Error = ErrorState::NotFound;

  if (fn)
  {
    String requestBody = "";
    
    if (_client.available())
    {
      Serial.println("  Read request body");
      while (_client.available()){
        requestBody += (char)_client.read();
        delay(0);
      }
    }

    Serial.print("  Call handler... ");
    response = fn(requestBody);
    Serial.println("done");
  }

  if (response.Error != ErrorState::None)
  {
    _errorState = response.Error;
    return;
  }

  clearClientBuffer();
  _client.println("HTTP/1.1 200 OK");
  switch (response.Type)
  {
  case ResponseType::Json:
    _client.println("Content-Type: application/json");
    break;
  case ResponseType::Text:
    _client.println("Content-Type: text/plain");
    break;
  case ResponseType::Empty:
    _client.println("Connection: Closed\r\n");
    return;

  default:
    Serial.printf("response.Type out of bounds: %d\r\n", _processStep);
    _errorState = ErrorState::InternalServerError;
    return;
  }
  _client.printf("Content-Length: %d\r\n\r\n", response.Body.length());
  _client.println(response.Body);
}
