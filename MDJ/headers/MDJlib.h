#ifndef MDJLIB_H_
#define MDJLIB_H_

#include "kemmens/config.h"
#include "kemmens/ThreadPool.h"
#include "kemmens/CommandInterpreter.h"
#include "kemmens/SocketClient.h"
#include "kemmens/SocketServer.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "Config.h"

ThreadPool* threadPool;
//Macros

void configurar();

void freeGlobals();

void startServer();

void initGlobals();

void* CommandIAm (int argc, char** args, char* comando, void* datos);

void* Command_ls (int argc, char** args, char* comando, void* datos);
void* Command_cat (int argc, char** args, char* comando, void* datos);
void* Command_md5 (int argc, char** args, char* comando, void* datos);
void* Command_cd (int argc, char** args, char* comando, void* datos);
void *CommandQuit (int argC, char** args, char* callingLine, void* extraData);

int cd(char* path, char** tmp);

void clientConnected(int socket);

void clientDisconnected(int unSocket);

void onPacketArrived(int socket, int tipoMensaje, void* datos);

void* postDo(char* cmd, char* sep, void* args, bool fired);

void ClientError(int socketID, int errorCode);

#endif /* MDJLIB_H_ */