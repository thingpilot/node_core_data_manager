/**
  * @file    DataManager.cpp
  * @version 1.0.0
  * @author  Rafaella Neofytou, Adam Mitchell
  * @brief   C++ file of the DataManager. Provides a very lightweight filesystem to facilitate the
  *          storage of arbritrary file types
  */

/** Includes
 */
#include "DataManager.h"


DataManager::DataManager()
{

}


DataManager::~DataManager()
{

}


uint16_t DataManager::get_max_types()
{
	return (uint16_t)TYPE_STORE_LENGTH - sizeof(DataManager_FileSystem::FileType_t);
}


int DataManager::get_max_records()
{
	return (int)RECORD_STORE_LENGTH - sizeof(DataManager_FileSystem::FileRecord_t);
}


int DataManager::get_storage_size_bytes()
{
	return (int)STORAGE_LENGTH;
}

