#ifndef KEMMENS_SERIALIZATION_H_
#define KEMMENS_SERIALIZATION_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//------ESTRUCTURAS EMPLEADAS------//

struct
{
	//Define el LARGO del campo data en bytes.
	uint32_t size;
	//Los datos a serializar.
	void* data;
} typedef SerializedPart;


/**
 *		Estructura que sirve como paquete para agrupar dos campos de datos, sus tamanios, y el tamanio total.
 *		La idea de los paquetes es guardar los datos a enviar ahi, serializarlos y enviar la cadena serializada
 *		CAMPOS:
 *			f1_length: Longitud del primer campo, en bytes (sin contar el \0 final)
 *			field1: Cadena de datos del primer campo
 *			f2_length: Longitud del segundo campo, en bytes (sin contar el \0 final)
 *			field2: Cadena de datos del segundo campo
 *			totalSize: Longitud total del paquete, en bytes (suma los largos de los campos, y el tamanio de sus lengths)
 */
typedef struct {
	uint32_t f1_length;
	char* field1;
	uint32_t f2_length;
	char* field2;
	uint32_t totalSize;
} TwoFieldedPackage;

/**
 *		Estructura que sirve como paquete para agrupar tres campos de datos, sus tamanios, y el tamanio total.
 *		La idea de los paquetes es guardar los datos a enviar ahi, serializarlos y enviar la cadena serializada
 *		CAMPOS:
 *			f1_length: Longitud del primer campo, en bytes (sin contar el \0 final)
 *			field1: Cadena de datos del primer campo
 *			f2_length: Longitud del segundo campo, en bytes (sin contar el \0 final)
 *			field2: Cadena de datos del segundo campo
 *			f3_length: Longitud del tercer campo, en bytes (sin contar el \0 final)
 *			field3: Cadena de datos del tercer campo
 *			totalSize: Longitud total del paquete, en bytes (suma los largos de los campos, y el tamanio de sus lengths)
 */
typedef struct {
	uint32_t f1_length;
	char* field1;
	uint32_t f2_length;
	char* field2;
	uint32_t f3_length;
	char* field3;
	uint32_t totalSize;
} ThreeFieldedPackage;

/**
 *		Estructura que sirve como paquete para agrupar cuatro campos de datos, sus tamanios, y el tamanio total.
 *		La idea de los paquetes es guardar los datos a enviar ahi, serializarlos y enviar la cadena serializada
 *		CAMPOS:
 *			f1_length: Longitud del primer campo, en bytes (sin contar el \0 final)
 *			field1: Cadena de datos del primer campo
 *			f2_length: Longitud del segundo campo, en bytes (sin contar el \0 final)
 *			field2: Cadena de datos del segundo campo
 *			f3_length: Longitud del tercer campo, en bytes (sin contar el \0 final)
 *			field3: Cadena de datos del tercer campo
 *			f4_length: Longitud del cuarto campo, en bytes (sin contar el \0 final)
 *			field4: Cadena de datos del cuarto campo
 *			totalSize: Longitud total del paquete, en bytes (suma los largos de los campos, y el tamanio de sus lengths)
 */
typedef struct {
	uint32_t f1_length;
	char* field1;
	uint32_t f2_length;
	char* field2;
	uint32_t f3_length;
	char* field3;
	uint32_t f4_length;
	char* field4;
	uint32_t totalSize;
} FourFieldedPackage;

//------FUNCIONES DEFINIDAS------//

/*
 * 	ACCION: Deserializar los campos de la cadena serializada que recibi con el getData y guardarlos en un array
 * 			NOTA: Esta funcion debe llamarse luego de recibir el ContentHeader
 * 	PARAMETROS:
 * 		serializedContent: Cadena serializada que quiero deserializar
 * 		dataArray: Array donde ir guardando los campos que voy deserializando (sin su longitud)
 */
void SocketCommons_DeserializeContent(char* serializedContent, void** dataArray);

/*
 * 		Permite serializar cualquier tipo de datos.
 *
 * 		Params:
 * 			fieldCount = Cantidad de SerializedPart que se van a recibir.
 * 			... = los SerializedPart separados por coma.
 *
 * 		Ejemplo:
 *
 * 		int* i1 = (int*)malloc(4);
 *		*i1 = 1;
 *		char* i2 = (char*)malloc(6);
 *		strcpy(i2, "hola!");
 *
 *		SerializedPart p1 = {.size = 4, .data = i1};
 *		SerializedPart p2 = {.size = strlen(i2)+1, .data = i2};
 *
 *		void* packet = Serialization_Serialize(2, p1, p2);
 *
 * 		El paquete serializado posee la siguiente estructura:
 *
 * 		+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-------+
 * 		|	size1   |	data1	| 	size2	|	data2	|	.....	|	sizeN	|	dataN	|	0	|
 * 		+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-------+
 *
 * 		Tanto "size" como "data" deben estar definidos por SerializedPart como se ve en el ejemplo. El orden de grabado en el paquete es el mismo que el orden de llegada de los parametros.
 *
 * 		Los paquetes SIEMPRE finalizan con un 0 a modo de determinar cuando se terminan de grabar datos.
 *
 */
void* Serialization_Serialize(int fieldCount, ...);

#endif /* KEMMENS_SERIALIZATION_H_ */
