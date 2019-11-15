/**
  * @file    DataManager_FileSystem.h
  * @version 0.3.0
  * @author  Adam Mitchell
  * @brief   Building blocks of a very lightweight filesystem
  */

/** Define to prevent recursive inclusion
 */
#pragma once

/** Includes 
 */
#include <mbed.h>

/** Collection of parameters that enable the management of a variety
 *  of persistent storage media and storage of 'files' within
 */
namespace DataManager_FileSystem
{
    /** 4 byte value used to determine whether or not the filesystem
     *  has been initialised
     */
    static const uint32_t INITIALISED = 0b01101001010110101100110001011100;

    /** Struct used to store useful global parameters
     */
    union GlobalStats_t
    {
        struct
        {
            uint16_t next_available_address;
            uint16_t space_remaining;
            uint32_t initialised;
        } parameters;

        char data[sizeof(GlobalStats_t::parameters)];
    };

    /** Struct used to store parameters that enable the storage,
     *  modification and deletion of entries within a 'file'
     */
	union File_t
    {
        struct 
        {
            uint16_t length_bytes;
            uint16_t file_start_address;
            uint16_t file_end_address;
            uint16_t next_available_address;
            uint8_t filename;
            uint8_t valid;
        } parameters;

        char data[sizeof(File_t::parameters)];
    };

    enum
    {
        FILE_TABLE_FULL                  = 20,
        FILE_INVALID_NAME                = 21
    };

    enum
    {
        FILE_ENTRY_LENGTH_MISMATCH       = 30,
        FILE_ENTRY_FULL                  = 31,
        FILE_ENTRY_INVALID_INDEX         = 32
    };
}