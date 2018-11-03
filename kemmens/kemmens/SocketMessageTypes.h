#ifndef KEMMENS_SOCKETMESSAGETYPES_H_
#define KEMMENS_SOCKETMESSAGETYPES_H_

/*	Definicion de tipos de mensajes para transferencias de datos por TCPIP
 */

//Message Types

#define MESSAGETYPE_STRING	1		//Strings puras
#define MESSAGETYPE_INT	2			//INTS de longitud variable
#define MESSAGETYPE_VOIDPOINTER 3	//void*, para los packets

//PROTOCOLO MDJ (1)
#define MESSAGETYPE_MDJ_CHECKFILE 	10	//Comando: Validar Archivo
#define MESSAGETYPE_MDJ_CREATEFILE	11	//Comando: Crear Archivo
#define MESSAGETYPE_MDJ_GETDATA		12	//Comando: Obtener bytes de archivo
#define MESSAGETYPE_MDJ_PUTDATA		13	//Comando: Grabar datos a archivo
#define MESSAGETYPE_MDJ_DELETEFILE	14	//Comando: Eliminar un archivo

//PROTOCOLO DAM (2)

#define MESSAGETYPE_DAM_ABRIR	20 //Comando: abrir <pathFile>
#define MESSAGETYPE_DAM_FLUSH	21 //Comando flush <pathFile>


#endif /* KEMMENS_SOCKETMESSAGETYPES_H_ */
