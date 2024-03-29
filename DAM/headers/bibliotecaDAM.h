#ifndef BIBLIOTECADAM_H_
#define BIBLIOTECADAM_H_

#include "kemmens/config.h"
#include "kemmens/ThreadPool.h"
#include "kemmens/CommandInterpreter.h"
#include "kemmens/SocketClient.h"
#include "kemmens/SocketCommons.h"
#include "kemmens/SocketServer.h"
#include "kemmens/SocketMessageTypes.h"
#include "Config.h"
#include "DAMInterface.h"


//-------------ESTRUCTURAS Y VARIABLES GLOBALES-------------//

ThreadPool* threadPool;		//"ThreadPool" para encolar tareas e hilos
t_list* cpus;					//Lista de CPUs conectadas

//-------------FUNCIONES-------------//

/*
 * 	ACCION: Guarda los datos del archivo de configuracion en la estructura global
 */
void configurar();

/*
 * 	ACCION: Configura segun el archivo, inicializa el CommandInterpreter, y crea el ThreadPool
 */
void inicializarVariablesGlobales();

/*
 * 	ACCION: Libera la memoria ocupada por la configuracion y el pozo
 */
void liberarVariablesGlobales();

/*
 * 	ACCION: Conectarse como cliente a un proceso y loguearlo; mandar un "iam ElDiego" como handshake
 * 	PARAMETROS:
 * 		ip:	IP del proceso al cual me quiero conectar
 * 		puerto: Puerto del proceso al cual me quiero conectar
 * 		nombreProceso: Nombre informal del proceso, para loguearlo
 */
int conectarAProceso(char* ip, char* puerto, char* nombreProceso);

/*
 * 	ACCION: Funcion threadeable para quedarme esperando un mensaje como respuesta
 * 	PARAMETROS:
 * 		socket: Socket a traves del cual espero la respuesta
 */
void esperarRespuesta(void* socket);

/*
 * 	ACCION: Levanta un servidor, lo pone a escuchar, y registra los listeners a hacer segun las acciones;
 * 			define el comportamiento al tener que encolar trabajos en el pozo
 */
void levantarServidor();

/*
 * 	ACCION: Funcion a realizar ante la lectura del comando iam, para el Command Interpreter
 * 	PARAMETROS:
 * 		argc: Cantidad de argumentos del comando, sin contar el nombre del mismo
 * 		args: Array de cadenas con los argumentos; args[0] es el comando en si
 * 		comando: Linea entera de comando, literal
 * 		datos: Datos extra, miscelanea
 */
void* comandoIAm (int argc, char** args, char* comando, void* datos);

/*
 * 	ACCION: Funcion a realizar al conectarse un cliente (CPU) al servidor levantado; logea y muestra por pantalla
 * 	PARAMETROS:
 * 		socket: Socket a traves del cual se establecio la comunicacion con el CPU (por el cual se hizo el accept)
 */
void clienteConectado(int socket);

/*
 * 	ACCION: Funcion a realizar al desconectarse un cliente (CPU) del servidor levantado; logea y muestra por pantalla
 * 	PARAMETROS:
 * 		unSocket: Socket a traves del cual se llevaba a cabo la comunicacion con el CPU desconectado
 */
void clienteDesconectado(int unSocket);

/*
 * 	ACCION: Funcion a realizar al recibir un cierto mensaje a traves de un cierto socket; segun el tipo de mensaje,
 * 			toma distintas medidas (e incluso lo encola en el PoolThread)
 * 	PARAMETROS:
 * 		socket: Socket a traves del cual llega el paquete
 * 		tipoMensaje: Tipo de mensaje del paquete que recien llego; determina las acciones a tomar
 * 		datos: Paquete recibido, como linea de comando entera
 */
void llegoUnPaquete(int socketID, int message_type, void* datos, int message_length);

/*
 *
 */
void* aRealizar(char* cmd, char* sep, void* args, bool fired);

#endif /* BIBLIOTECADAM_H_ */
