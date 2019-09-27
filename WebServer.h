/*
  WebServer.h - Lightweight HTTP Webserver.
  Copyright 2019, SytheZN, All rights reserved.
*/
#ifndef _WebServer_h
#define _WebServer_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>

#define WEBSERVER_FILE_BUFFER_LENGTH 128
#define READ_TIMEOUT 500

enum class ErrorState : uint8_t
{
  None = 0,
  ReadTimeout = 1,
  BadRequest = 2,
  MethodNotAllowed = 3,
  NotAcceptable = 4,
  Conflict = 5,
  NotFound = 6,
  InternalServerError = 7,
};

enum class ResponseType : uint8_t
{
  Empty = 0,
  Json = 1,
  Text = 2
};

class ApiMethodResponse
{
public:
  ErrorState Error = ErrorState::None;
  ResponseType Type = ResponseType::Empty;
  String Body = "";
};

class ApiMethod
{
public:
  typedef std::function<ApiMethodResponse(String&)> CallbackFunction;
  CallbackFunction Callback;
  String Path;
};

class WebServer
{
public:
  WebServer(WiFiServer *server);
  bool handle();

  void SetGetHandlers(ApiMethod *apiMethods, uint8_t count);
  void SetPutHandlers(ApiMethod *apiMethods, uint8_t count);
  void SetPostHandlers(ApiMethod *apiMethods, uint8_t count);
  void SetDeleteHandlers(ApiMethod *apiMethods, uint8_t count);

private:
  void loop();

  void handleError();
  void writeError(const String response);
  void resetState();

  void readClientRequestHeader();
  void clearClientBuffer();
  bool isFileNameLegal(String path);

  void parseRequestHeader();
  void selectRequestMethod();

  void selectRequestPath_GET();
  void selectRequestPath_PUT();
  void selectRequestPath_POST();
  void selectRequestPath_DELETE();

  void serve_GET_fileList();
  void serve_GET_file();
  void serve_PUT_file();
  void serve_DELETE_file();

  void serve_api(ApiMethod::CallbackFunction fn);

  ApiMethod *_api_GETs;
  uint8_t _api_GETsLength;
  ApiMethod *_api_PUTs;
  uint8_t _api_PUTsLength;
  ApiMethod *_api_POSTs;
  uint8_t _api_POSTsLength;
  ApiMethod *_api_DELETEs;
  uint8_t _api_DELETEsLength;

  enum class ProcessStep : uint8_t
  {
    AwaitClient = 0,
    GetRequestHeader = 1,
    ParseRequestHeader = 2,
    SelectRequestMethodHandler = 3,

    RequestMethod_GET = 10,
    ProcessRequest_GET = 11,
    EndRequest_GET = 12,

    RequestMethod_POST = 20,
    ProcessRequest_POST = 21,
    EndRequest_POST = 22,

    RequestMethod_PUT = 30,
    ProcessRequest_PUT = 31,
    EndRequest_PUT = 32,

    RequestMethod_DELETE = 40,
    ProcessRequest_DELETE = 41,
    EndRequest_DELETE = 42,
  };

  WiFiServer *_server;

  WiFiClient _client;
  String _requestHeader = "";
  String _requestHeaderParts[3];
  ProcessStep _processStep = ProcessStep::AwaitClient;
  ErrorState _errorState = ErrorState::None;
  bool _errorHandled = false;
};

#endif
