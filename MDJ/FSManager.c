#include "headers/FSManager.h"
#include <string.h>
t_bitarray* FSBitMap = NULL;


static void FIFA_FlushBitmap()
{
	FILE *fp;
	fp = fopen(config->bitmapFile, "w+b");

	pthread_mutex_lock(&bitmapLock);
	fwrite(FSBitMap->bitarray, FSBitMap->size, 1, fp);
	pthread_mutex_unlock(&bitmapLock);
	fclose(fp);
}

static int FIFA_CalculateBlockAmount(int bytes)
{
	return ceil(bytes / (float)config->tamanioBloque);
}

static void FIFA_BlockPutContent(int blockNum, int offset, void* content, int len)
{
	Logger_Log(LOG_DEBUG, "FIFA -> Solicitud de escritura al bloque %d", blockNum);

	char* blockFile = StringUtils_Format("%s%d.bin", config->blocksPath, blockNum);

	FILE* fd = fopen(blockFile, "r+b");
	if(!fd)
	{
		FILE* file = fopen(blockFile, "wb");
		if(!file)
		{
			return;
		}
		fclose(file);

		fd = fopen(blockFile, "r+b");
	}

	if(!fd)
	{
		return;
	}

	if(len != 0)
	{
		fseek(fd, offset, SEEK_SET);

		fwrite(content, len, 1, fd);
	}

	fclose(fd);

	free(blockFile);
}

/*
 * 	Comprueba si un bloque no esta usado y lo reserva en forma atomica con un mutex.
 */
static bool FIFA_ReserveBlockIfNotUsed(int blockNum)
{
	pthread_mutex_lock(&bitmapLock);
	bool val = bitarray_test_bit(FSBitMap, blockNum);

	if(!val)
		bitarray_set_bit(FSBitMap, blockNum);
	pthread_mutex_unlock(&bitmapLock);
	return !val;
}

static int FIFA_ReserveNextFreeBlock()
{
	for(int i = 0 ; i < config->cantidadBloques;i++)
	{
		if(FIFA_ReserveBlockIfNotUsed(i))
			return i;
	}

	return -1;
}

static char* FIFA_GetFullPath(char* path)
{
	return StringUtils_Format("%s%s", config->filesPath, path);
}

static void FIFA_InitBitmapFile()
{
	int cantBytes = ceil(config->cantidadBloques / (float)8);

	Logger_Log(LOG_DEBUG, "FIFA -> Creando BITMAP con cantidad de bloque (bits): %d, cantidad de bytes: %d, cantidad de bits totales: %d",
							config->cantidadBloques, cantBytes, cantBytes*8);

	char* base = (char*)malloc(cantBytes);

	memset(base, 0b000000000, cantBytes);

	pthread_mutex_lock(&bitmapLock);
	FSBitMap = bitarray_create_with_mode(base, cantBytes, MSB_FIRST);
	pthread_mutex_unlock(&bitmapLock);

	Logger_Log(LOG_DEBUG, "FIFA -> BITMAP creado, cantidad total de bits en bitmap: %d", bitarray_get_max_bit(FSBitMap));
	FIFA_FlushBitmap();
}

static bool FIFA_FileExists(char* path)
{
	return access( path, F_OK ) != -1;
}

static int FIFA_GetBlockFromOffset(int offset)
{
	return ceil(offset/(float)config->tamanioBloque) - 1; //Nuestros bloques empiezan en 0
}

static int FIFA_GetFirstByteFromOffset(int offset)
{
	//El offset que nos vamos a correr adentro de nuestro bloque.
	return offset % config->tamanioBloque;
}

static void FIFA_FreeBitmap()
{
	pthread_mutex_lock(&bitmapLock);
	if(FSBitMap != NULL)
	{
		if(FSBitMap->bitarray != NULL)
			free(FSBitMap->bitarray);

		bitarray_destroy(FSBitMap);
	}
	pthread_mutex_unlock(&bitmapLock);
	pthread_mutex_destroy(&bitmapLock);
}
/*
static void FIFA_SetUsedBlock(int blockNum)
{
	pthread_mutex_lock(&bitmapLock);
	bitarray_set_bit(FSBitMap, blockNum);
	pthread_mutex_unlock(&bitmapLock);
}
*/
static void FIFA_FreeBlock(int blockNum)
{
	pthread_mutex_lock(&bitmapLock);
	bitarray_clean_bit(FSBitMap, blockNum);
	pthread_mutex_unlock(&bitmapLock);
	Logger_Log(LOG_DEBUG, "FIFA -> Bloque %d liberado.", blockNum);
}

static bool FIFA_IsBlockUsed(int blockNum)
{
	pthread_mutex_lock(&bitmapLock);
	bool val = bitarray_test_bit(FSBitMap, blockNum);
	pthread_mutex_unlock(&bitmapLock);
	return val;
}

static void FIFA_MkDirFrom(char* path, char* from)
{
	Logger_Log(LOG_DEBUG, "FIFA -> mkdir recursivo de %s", path);
	char** paths = string_split(path, "/");
	char* tmp = (char*)malloc( string_length(from) + 1 );
	strcpy(tmp, from);

	int i = 0;
	while (paths[i] != NULL) {
		string_append(&tmp, "/");
		string_append(&tmp, paths[i]);

		Logger_Log(LOG_DEBUG, "FIFA -> mkdir : %s", tmp);

		mkdir(tmp, 0700);
		free(paths[i]);
		i++;
	}

	free(tmp);
	free(paths);
}

static void FIFA_Init()
{
	Logger_Log(LOG_DEBUG, "FIFA -> Inicializando File System...");
	FIFA_MkDirFrom(config->puntoMontaje, "");
	mkdir(config->blocksPath, 0700);
	mkdir(config->filesPath, 0700);
	for(int i = 0; i < config->cantidadBloques;i++) //creamos bloques vacios si es necesario
	{
		FIFA_BlockPutContent(i, 0, 0, 0);
	}
	pthread_mutex_init(&bitmapLock, NULL);
}

static void FIFA_ReadBitmap()
{
	Logger_Log(LOG_DEBUG, "FIFA -> Cargando Bitmap...");
	FILE *fp;
	fp = fopen(config->bitmapFile, "rb");

	if(!fp)
	{
		Logger_Log(LOG_DEBUG, "FIFA -> BITMAP no encontrado, inicializando...");
		FIFA_InitBitmapFile();
		fp = fopen(config->bitmapFile, "rb");
		if(!fp)
		{
			Logger_Log(LOG_ERROR, "FIFA -> ERROR AL ABRIR BITMAP DESDE ARCHIVO!");
			return;
		}
	}

	char* buff = (char*)malloc(config->cantidadBloques);

	fread(buff, config->cantidadBloques, 1, fp);

	fclose(fp);

	FIFA_FreeBitmap();

	pthread_mutex_lock(&bitmapLock);
	FSBitMap = bitarray_create_with_mode(buff, ceil(config->cantidadBloques / (float)8), MSB_FIRST);
	pthread_mutex_unlock(&bitmapLock);
}

static int* FIFA_ReserveBlocks(int cantBloques)
{
	int* bloques = (int*)malloc(sizeof(int) * cantBloques);

	int tmp;
	for(int i = 0 ; i < cantBloques;i++)
	{
		//ya arreglado: Posible bug: si se intenta crear un archivo que necesita X bloques pero solo hay Y libres (Y < X) los Y bloques se van a reservar pero la creacion va a fallar poque no hay X.
		tmp = FIFA_ReserveNextFreeBlock();
		if(tmp == -1) // no hay mas bloques
		{
			Logger_Log(LOG_DEBUG, "FIFA -> Se intentaron reservar mas bloques que los dispnibles! La creacion/ampliacion fallo.");
			if(i > 0) //Ya reservamos al menos un bloque, debemos liberarlo/s porque no lo/s vamos a usar.
			{
				Logger_Log(LOG_DEBUG, "FIFA -> Haciendo rollback de intento de creacion... liberando bloques...");
				for(int j = 0 ; j < i ; j++)
				{
					FIFA_FreeBlock( bloques[j] );
				}
			}

			free(bloques);
			return NULL;
		}

		Logger_Log(LOG_DEBUG, "FIFA -> Bloque %d reservado.", tmp);

		bloques[i] = tmp;
	}

	return bloques;
}

//General Functions

t_config* FIFA_OpenFile(char* path)
{
	char* fullPath = FIFA_GetFullPath(path);
	t_config* metadata = NULL;
	DIR* dir = opendir(fullPath);
	if (!dir)
	{
		metadata = config_create(fullPath);
	} else
		closedir(dir);

	free(fullPath);

	if(metadata == NULL)
	{
		Logger_Log(LOG_DEBUG, "FIFA: Attempt to open '%s' failed. File may not exist.", path);
		return NULL;
	}

	return metadata;
}

void FIFA_PrintBitmap()
{
	for(int i = 0; i < config->cantidadBloques; i++)
	{
		printf("--------------------------------\n");
		printf("\t Bit : %d = %d \n", i, FIFA_IsBlockUsed(i));
		printf("--------------------------------\n");
	}
}


void FIFA_MkDir(char* path)
{
	FIFA_MkDirFrom(path, config->filesPath);
}

int FIFA_WriteFile(char* path, int offset, int size, void* data)
{
	char* fullPath = FIFA_GetFullPath(path);

	Logger_Log(LOG_DEBUG, "FIFA -> Solicitud de modificacion de archivo '%s'.", path);

	if( !FIFA_FileExists(fullPath) ) {
		Logger_Log(LOG_DEBUG, "FIFA -> Write file no puede ejecutar %s no existe", fullPath);
		free(fullPath);
		return FILE_NOT_EXISTS;
	}

	t_config* metadata = FIFA_OpenFile(path);

	if(metadata == NULL)
	{
		Logger_Log(LOG_ERROR, "FIFA -> Error WriteFile al abrir archivo de metadata para archivo %s", fullPath);
		free(fullPath);
		return METADATA_OPEN_ERROR;
	}

	int fileSize = config_get_int_value(metadata, "TAMANIO");

	char* blocksAsString = config_get_string_value(metadata, "BLOQUES");

	int blockCount = StringUtils_CountOccurrences(blocksAsString, ",") + 1;

	int availableSpaceInLastBlock = (blockCount * config->tamanioBloque) - fileSize;

	int neededSize = offset + size;

	if(neededSize > fileSize)
	{
		Logger_Log(LOG_DEBUG, "FIFA -> El archivo '%s' va a intentar ampliarse.", path);
		int newBytes = neededSize - fileSize;
		char* tmp;
		tmp = string_itoa(neededSize);
		config_set_value(metadata, "TAMANIO", tmp);
		free(tmp);

		if(newBytes > availableSpaceInLastBlock) //no nos alcanza el espacio que sobro en el ultimo bloque del archivo.
		{
			int reservar = FIFA_CalculateBlockAmount( newBytes );
			int* newBlocks = FIFA_ReserveBlocks( reservar );

			if(newBlocks == NULL)
			{
				config_destroy(metadata);
				free(fullPath);
				return INSUFFICIENT_SPACE;
			}

			char* temp = strdup(blocksAsString);
			temp[ strlen(temp) - 1 ] = ',';

			char* tmp = StringUtils_ArrayFromInts(newBlocks, reservar, false, true);
			string_append(&temp, tmp);
			config_set_value(metadata, "BLOQUES", temp);

			//necesitamos la nueva direccion de memoria.
			blocksAsString = config_get_string_value(metadata, "BLOQUES");

			free(temp);
			free(tmp);
			free(newBlocks);
		}
	}

	char** blocks = string_get_string_as_array(blocksAsString);

	int primerBloqueIndex = FIFA_GetBlockFromOffset(offset);

	int primerByte = FIFA_GetFirstByteFromOffset(offset);

	//Por el offset que definimos si por ejemplo tenemos un tamaño 4 y offset 4 caeriamos en el blqoue 1° pero tendriamos que ir al 2°
	//Ademas el si el offset el 0, el prime block que va a ser -1, por lo que debemos incrementar para que sea 0.
	if(primerByte == 0)
		primerBloqueIndex ++;

	blockCount = StringUtils_ArraySize(blocks);

	int tam = config->tamanioBloque - primerByte;

	if(size < tam)
		tam = size;

	void* dataCopy = data;
	int sizeCopy = size;

	for(int i = primerBloqueIndex; i < blockCount;i++)
	{
		FIFA_BlockPutContent( atoi(blocks[i]), primerByte, dataCopy, tam);

		if(sizeCopy - tam <= 0)
			break;

		if(i == primerBloqueIndex)
			primerByte = 0;

		dataCopy += tam; //TODO: Actualizamos el pointer desde donde copiar, verificar que la cuenta este bien hecha y si efectuvamente es <= porque tal vez si es = tira segfault.
		sizeCopy -= tam;

		if(sizeCopy > config->tamanioBloque)
		{
			tam = config->tamanioBloque;
		} else
			tam = sizeCopy;
	}


	config_save(metadata);
	StringUtils_FreeArray(blocks);
	config_destroy(metadata);
	FIFA_FlushBitmap();
	free(fullPath);
	return OPERATION_SUCCESSFUL;
}

int FIFA_CreateFile(char* path, int newLines)
{
	char* fullPath = FIFA_GetFullPath(path);

	Logger_Log(LOG_DEBUG, "FIFA -> Petition to create file at '%s'", fullPath);

	if( FIFA_FileExists(fullPath) ) {
		Logger_Log(LOG_DEBUG, "FIFA -> No se creara el archivo %s. El archivo ya existe.", fullPath);
	    free(fullPath);
	    return EXISTING_FILE;
	}

	int cantBloques = ceil(newLines / (float)config->tamanioBloque);

	Logger_Log(LOG_DEBUG, "FIFA -> Cantidad de bloques requererida para creacion: %d", cantBloques);

	int* bloques;

	bloques = FIFA_ReserveBlocks(cantBloques);

	if(bloques == NULL)
	{
		free(fullPath);
		return INSUFFICIENT_SPACE;
	}

	char* blocksCharArray = StringUtils_ArrayFromInts(bloques, cantBloques, true, true);

	Logger_Log(LOG_DEBUG, "FIFA -> Bloques asignados a nuevo archivo: %s", blocksCharArray);

	//Creamos las rutas
	int last = StringUtils_LastIndexOf(path, '/');

	char* folders = string_substring_until(path, last + 1);

	FIFA_MkDir(folders);

	free(folders);

	void fallar(int* bloques, char* path, char* blocks, int cant)
	{
		for(int i = 0; i < cant;i++)
		{
			FIFA_FreeBlock( bloques[i] );
		}
		free(path);
		free(blocks);
		free(bloques);
	}

	FILE* file = fopen(fullPath, "w"); //Creamos el archivo para t_config ya que no puede crearlo solo.
	if(!file)
	{
		fallar(bloques, fullPath, blocksCharArray, cantBloques);
		return METADATA_CREATE_ERROR;
	}
	fclose(file);

	t_config* metadata = FIFA_OpenFile(path);

	if(metadata == NULL)
	{
		Logger_Log(LOG_ERROR, "FIFA -> Error al abrir archivo de metadata para archivo %s", fullPath);
		fallar(bloques, fullPath, blocksCharArray, cantBloques);
		return METADATA_OPEN_ERROR;
	}

	char* size = string_itoa(newLines);

	config_set_value(metadata, "TAMANIO", size);

	free(size);

	config_set_value(metadata, "BLOQUES", blocksCharArray);
	free(blocksCharArray);

	config_save(metadata);
	config_destroy(metadata);

	int cant = 0;
	char* content;
	for(int i = 0; i < cantBloques; i++)
	{
		if(config->tamanioBloque < newLines)
			cant = config->tamanioBloque;
		else
			cant = newLines;

		content = string_repeat('\n', cant);

		newLines -= cant;

		FIFA_BlockPutContent(bloques[i], 0, content, cant);

		free(content);
	}

	free(bloques);
	free(fullPath);
	FIFA_FlushBitmap();
	return OPERATION_SUCCESSFUL;
}

bool FIFA_IsFileValid(char* path)
{
	t_config* metadata = FIFA_OpenFile(path);

	if(metadata == NULL)
		return false;

	config_destroy(metadata);
	return true;
}

char* FIFA_ReadFile(char* path, int offset, int size, int* amountCopied)
{
	t_config* metadata = FIFA_OpenFile(path);

	if(metadata == NULL)
		return NULL;

	/*
	 * +------------+-----------+
	 * |	0123	|	4567	|
	 * +------------+-----------+
	 */

	char** bloques = config_get_array_value(metadata, "BLOQUES");

	// Nos fijamos en que bloque empezar a buscar los bytes a copiar
	int primerBloqueIndex = FIFA_GetBlockFromOffset(offset);

	//El offset que nos vamos a correr adentro de nuestro bloque.
	int primerByte = FIFA_GetFirstByteFromOffset(offset);

	//Por el offset que definimos si por ejemplo tenemos un tamaño 4 y offset 4 caeriamos en el blqoue 1° pero tendriamos que ir al 2°
	//Ademas el si el offset el 0, el prime block que va a ser -1, por lo que debemos incrementar para que sea 0.
	if(primerByte == 0)
		primerBloqueIndex ++;

	int cantBloques = StringUtils_ArraySize(bloques);

	Logger_Log(LOG_DEBUG, "\nFILE SYSTEM ACCESS \n\t FilesPath: '%s'\n\t path = '%s' \n\t primerBloqueIndex = %d \n\t primerByte = %d \n\t cantBloques = %d \n\t size = %d" ,
			config->filesPath, path, primerBloqueIndex, primerByte, cantBloques, size);

	char* buffer = malloc(1);
	int copiado = 0;
	int realCopiado = 0;
	int tam;
	FILE *fp;
	char* blockFilePath;

	for(int i = primerBloqueIndex; i < cantBloques;i++)
	{
		blockFilePath = StringUtils_Format("%s%s%s", config->blocksPath, bloques[i], ".bin");

		Logger_Log(LOG_DEBUG, "\nBLOCK ACCESS \n\t blockFilePath = '%s' \n\t" ,
					blockFilePath);

		fp = fopen(blockFilePath, "rb");

		if(primerByte != 0)
		{
			fseek ( fp , primerByte , SEEK_SET );
			tam = config->tamanioBloque - primerByte;
			primerByte = 0; //Despues de la primera vuelta el primer byte es siempre 0 porque hay que leer el archivo desde el inicio.
		} else
			tam = config->tamanioBloque;

		copiado += tam;
		if(copiado > size)
			tam = size - realCopiado;

		buffer = realloc(buffer, realCopiado + tam);
		//el realCopiado lo va a dar fread porque puede ser que un bloque no este 100% lleno por lo que lo leemos por la mitad, fread va a cortar en EOF y nos va a decir cuanto copio realmentea, esto va a pasar porque size puede ser > que el tamaño del archivo
		realCopiado += fread(buffer + realCopiado, 1, tam, fp);

		fclose(fp);
		free(blockFilePath);
		if(copiado >= size)
			break;
	}
	config_destroy(metadata);
	StringUtils_FreeArray(bloques);

	if(amountCopied != NULL)
		*amountCopied = realCopiado;

	return buffer;
}

int FIFA_DeleteFile(char* path)
{
	t_config* metadata = FIFA_OpenFile(path);

	if(metadata == NULL)
		return FILE_NOT_EXISTS;

	char** bloques = config_get_array_value(metadata, "BLOQUES");

	int cantBloques = StringUtils_ArraySize(bloques);

	char* blockFilePath;
	FILE* tmpfile;
	for(int i = 0; i < cantBloques;i++)
	{
		blockFilePath = StringUtils_Format("%s%s%s", config->blocksPath, bloques[i], ".bin");

		Logger_Log(LOG_DEBUG, "FIFA -> Delete '%s' :: Limpiando y liberando bloque en '%s'" , path, blockFilePath);

		//No hay que borrar los bloques, solo dejarlos ahi con basura
		if((tmpfile = fopen(blockFilePath, "w")) != 0)
		{
			fclose(tmpfile);
			free(blockFilePath);
		} else
		{
			//Tenemos un bloque corrupto, va a quedar inutilizable en el filesystem :c
			Logger_Log(LOG_ERROR, "FIFA -> La eliminacion del bloque en '%s' ha fallado!", blockFilePath);
			free(blockFilePath);
			continue;
		}

		FIFA_FreeBlock( atoi(bloques[i]) );
	}

	Logger_Log(LOG_DEBUG, "FIFA -> Delete '%s' :: Bloques eliminados. Eliminando metadata..." , path);

	char* fullPath = FIFA_GetFullPath(path);
	if(remove(fullPath) != 0)
	{
		Logger_Log(LOG_ERROR, "FIFA -> La eliminacion de la metadata '%s' ha fallado!", fullPath);
	}
	free(fullPath);

	StringUtils_FreeArray(bloques);
	config_destroy(metadata);
	FIFA_FlushBitmap();
	return OPERATION_SUCCESSFUL;
}

void FIFA_Start()
{
	FIFA_Init();
	FIFA_ReadBitmap();
}

void FIFA_ShutDown()
{
	FIFA_FreeBitmap();
}
