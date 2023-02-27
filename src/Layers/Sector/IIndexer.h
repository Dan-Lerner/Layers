/*
 * IIndexer.h
 *
 *  Created on: 12 дек. 2019 г.
 *      Author: dan
 */

#ifndef IINDEXER_H_
#define IINDEXER_H_

#include "Layers/Macro.h"

namespace DLayers
{

class IndexContent;

class IIndexer
{
public:

	IIndexer() { };

	virtual ~IIndexer() { };

public:

	// Указатель на сектор, содержащий индексируемые сектора
	virtual IndexedSector* Container()	= 0;

	// Указатель на хранилище хедеров
	virtual IHeaderStore* HeaderStore()	= 0;

// Работа с блоками, содержащими несколько секторов

// !!! В реализации этих функций необходимо следить за последовательностью в цепочках

public:

	virtual bool CopyContent(D_INDEX indexFrom, D_INDEX indexTo, D_INDEX_COUNT count = 1)	= 0;

	virtual bool MoveContent(D_INDEX indexFrom, D_INDEX indexTo, D_INDEX_COUNT count = 1)	= 0;

	virtual bool ShiftContent(D_INDEX indexFrom, D_INDEX indexTo, D_INDEX_COUNT count = 1)	= 0;

	virtual bool ExchangeContent(D_INDEX index1, D_INDEX index2, D_INDEX_COUNT count = 1)	= 0;

	virtual bool RemoveContent(D_INDEX indexStart, D_INDEX_COUNT count = 1) 				= 0;

	// Сдвигает блок секторов на заданное смещение
	virtual bool MoveContentOnDelta(D_INDEX indexStart, D_INDEX indexEnd, D_OFFSET offsetDelta) = 0;

// Прямой указатель на индексируемый сектор
public:

	virtual void* ContentMemory(D_INDEX index) = 0;

// Запросы и уведомления от индексируемых секторов
public:

	// Вызываются в процессе добавления нового сектора
	virtual bool n_RequestContentAdd(IndexedSector* pSector, bool* pAnyPlace = NULL, D_INDEX* pIndexAfter = NULL) = 0;
	virtual bool n_ContentAdded(IndexedSector* pSector) = 0;

	// Вызываются при изменении размера сектора
	// sizeNew - запрашиваемый размер сектора целиком
	virtual bool n_RequestContentResize(IndexedSector* pSector, D_SIZE sizeNew) = 0;
	virtual bool n_ContentResized(IndexedSector* pSector) = 0;

	// Вызываются в процессе удаления сектора
	virtual bool n_RequestContentDelete(IndexedSector* pSector) = 0;
	virtual bool n_ContentDeleted(D_INDEX index, Location& location) = 0;

// Функции запроса свободного места
public:

	// Выделение места для новой записи

	// bAnyPlace - если true, то свободное место определяется сектором Fixed
	// indexAfter - если !bAnyPlace, то индекс выделяется после indexAfter
	// size - размер записи (в этом классе не используется)
	// location - расположение выделенного свободного места

	virtual D_INDEX RequestSpace(bool bAnyPlace, D_INDEX indexAfter, D_SIZE size, Location& location) = 0;

	// Запрос места size после сектора
	virtual bool RequestSpaceAfter(D_INDEX index, D_SIZE size, Location& location) = 0;

	// Запрос места size перед сектором
	virtual bool RequestSpaceBefore(D_INDEX index, D_SIZE size, Location& location) = 0;

// Service
public:

	// Возвращает полный размер сектора
	virtual D_SIZE ContentSize(D_INDEX index = D_RET_ERROR) = 0;

	// Получение расположения секторов в контейнере по индексу
	virtual bool GetContentLocation(D_INDEX index, Location& location, D_INDEX_COUNT count = 1) = 0;

	// Смещение сектора в контейнере по индексу
	virtual D_OFFSET ContentOffset(D_INDEX index) = 0;

	// Получение содержимого записи индекса
	virtual int GetIndexRecordContent(D_INDEX index, IndexContent* pIndexRecord) = 0;

// Работа с последовательностью индексов

// Возвращаемое значение:

// индекс в случае успеха
// D_RET_ERROR - если запрашиваемого индекса нет или в случае ошибки !!! Нужно дифференцировать

public:

	// Индекс первой записи
	virtual D_INDEX StartIndex()				= 0;

	// Индекс последней записи
	virtual D_INDEX EndIndex()					= 0;

	// Следующий индекс
	virtual D_INDEX NextIndex(D_INDEX index)	= 0;

	// Предыдущий индекс
	virtual D_INDEX PrevIndex(D_INDEX index)	= 0;

	// Первая удаленная запись после index
	virtual D_INDEX NextDeleted(D_INDEX index)	= 0;

	// Первая удаленная запись перед index
	virtual D_INDEX PrevDeleted(D_INDEX index)	= 0;

	// Первая валидная запись после index
	virtual D_INDEX NextValid(D_INDEX index)	= 0;

	// Первая валидная запись перед index
	virtual D_INDEX PrevValid(D_INDEX index)	= 0;
};

} /* namespace DLayers */

#endif /* IINDEXER_H_ */
