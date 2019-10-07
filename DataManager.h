/**
  * @file    DataManager.h
  * @version 1.0.0
  * @author  Rafaella Neofytou, Adam Mitchell
  * @brief   Header file of the DataManager. Provides a very lightweight filesystem to facilitate the
  *          storage of arbritrary file types
  */

/** Define to prevent recursive inclusion
 */
#pragma once

/** Includes 
 */
#include <mbed.h>
#include "board.h"

#if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0)

#include "STM24256.h"

#define PAGES 500
#define PAGE_SIZE_BYTES 64
#define TYPE_STORE_PAGES 2
#define RECORD_STORE_PAGES 125

#define TYPE_STORE_START_ADDRESS 0
#define TYPE_STORE_LENGTH PAGE_SIZE_BYTES * TYPE_STORE_PAGES
#define RECORD_STORE_START_ADDRESS TYPE_STORE_LENGTH
#define RECORD_STORE_LENGTH PAGE_SIZE_BYTES * RECORD_STORE_PAGES
#define STORAGE_START_ADDRESS TYPE_STORE_LENGTH + RECORD_STORE_LENGTH
#define STORAGE_LENGTH (PAGES * PAGE_SIZE_BYTES) - (STORAGE_START_ADDRESS)

namespace DataManager_FileSystem
{
	union FileType_t
		{
			struct 
			{
				int type_id;
				int length;
			} parameters;

			char data[sizeof(FileType_t::parameters)];
		};

	union FileRecord_t
	{
		struct
		{
			uint16_t start_address;
			uint16_t length_bytes;
			uint16_t record_id;
			uint8_t type_id;
			uint8_t UNUSED;
		} parameters;
		
		char data[sizeof(FileRecord_t::parameters)];
	};
}
#endif /* #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0) */


/** Base class for the Data Manager
 */ 
class DataManager
{

	public:

        #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0)
        DataManager(PinName write_control, PinName sda, PinName scl, int frequency_hz);
        #endif /* #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0) */

		~DataManager();

		uint16_t get_max_types();

		int get_max_records();

		int get_storage_size_bytes();


    private:

        #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0)
        STM24256 _storage;
        #endif /* #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0) */
        
};