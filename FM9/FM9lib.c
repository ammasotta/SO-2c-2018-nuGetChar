#include "headers/FM9lib.h"

void configurar() {
	Logger_Log(LOG_INFO, "FM9 -> Configurando.");

	settings = (Configuracion*) malloc(sizeof(Configuracion));

	char* campos[] = { "PUERTO", "MODO", "TAMANIO", "MAX_LINEA",
	NULL };

	t_config* archivoConfig = archivoConfigCrear(RUTA_CONFIG, campos);

	settings->puerto = archivoConfigSacarIntDe(archivoConfig, "PUERTO");
	if (string_equals_ignore_case("SEG",
			archivoConfigSacarStringDe(archivoConfig, "MODO")))
		settings->modo = SEG;
	else if (string_equals_ignore_case("TPI",
			archivoConfigSacarStringDe(archivoConfig, "MODO")))
		settings->modo = TPI;
	else if (string_equals_ignore_case("SPA",
			archivoConfigSacarStringDe(archivoConfig, "MODO")))
		settings->modo = SPA;
	else
		archivoConfigEsInvalido();
	settings->tamanio = archivoConfigSacarIntDe(archivoConfig, "TAMANIO");
	settings->max_linea = archivoConfigSacarIntDe(archivoConfig, "MAX_LINEA");
	if (settings->modo != SEG) {
		if (!archivoConfigTieneCampo(archivoConfig, "TAM_PAGINA")) {
			archivoConfigEsInvalido();
		} else {

			settings->tam_pagina = archivoConfigSacarIntDe(archivoConfig,
					"TAM_PAGINA");
		}
	}
	memoryFunctions = malloc(sizeof(t_memoryFunctions));
	switch (settings->modo) {
		case SEG:
			memoryFunctions->createStructures = createSegmentationStructures;
			memoryFunctions->freeStructures = freeSegmentationStructures;
			memoryFunctions->logicalAddressTranslation = addressTranslation_SEG;
			memoryFunctions->writeData = writeData_SEG;
			memoryFunctions->readData = readData_SEG;
			memoryFunctions->closeFile = closeFile_SEG;
			memoryFunctions->closeDTBFiles = closeDTBFiles_SEG;
			memoryFunctions->dump = dump_SEG;
			break;
		case SPA:
			memoryFunctions->createStructures = createPagedSegmentationStructures;
			memoryFunctions->freeStructures = freePagedSegmentationStructures;
			memoryFunctions->logicalAddressTranslation = addressTranslation_SPA;
			memoryFunctions->writeData = writeData_SPA;
			memoryFunctions->readData = readData_SPA;
			memoryFunctions->closeFile = closeFile_SPA;
			memoryFunctions->closeDTBFiles = closeDTBFiles_SPA;
			memoryFunctions->dump = dump_SPA;
			break;
		case TPI:
			memoryFunctions->createStructures = createIPTStructures;
			memoryFunctions->freeStructures = freeIPTStructures;
			memoryFunctions->logicalAddressTranslation = addressTranslation_TPI;
			memoryFunctions->writeData = writeData_TPI;
			memoryFunctions->readData = readData_TPI;
			memoryFunctions->closeFile = closeFile_TPI;
			memoryFunctions->closeDTBFiles = closeDTBFiles_TPI;
			memoryFunctions->dump = dump_TPI;
			break;
	}
	archivoConfigDestruir(archivoConfig);

	Logger_Log(LOG_INFO, "FM9 -> Configurado correctamente.");

}
