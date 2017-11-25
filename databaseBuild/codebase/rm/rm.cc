#include "rm.h"

RelationManager* RelationManager::instance()
{
	static RelationManager _rm;
	return(&_rm);
}


RelationManager::RelationManager()
{
	id = 0;
}


RelationManager::~RelationManager()
{
}


RC RelationManager::tablesRecordGenerator( const int &id, const string &tableName, const string &fileName, void *record )
{
	int	length1 = tableName.length();
	int	length2 = fileName.length();
	memset( record, 0, 1 );
	memcpy( (char *) record + 1, &id, 4 );
	memcpy( (char *) record + 5, &length1, 4 );
	memcpy( (char *) record + 9, tableName.c_str(), length1 );
	memcpy( (char *) record + 9 + length1, &length2, 4 );
	memcpy( (char *) record + 13 + length1, fileName.c_str(), length2 );
	return(0);
}


RC RelationManager::columnsRecordGenerator( const int &id, const string &attrName, const int &type, const int &len, const int &pos, void *record )
{
	int length = attrName.length();
	memset( record, 0, 1 );
	memcpy( (char *) record + 1, &id, 4 );
	memcpy( (char *) record + 5, &length, 4 );
	memcpy( (char *) record + 9, attrName.c_str(), length );
	memcpy( (char *) record + 9 + length, &type, 4 );
	memcpy( (char *) record + 13 + length, &len, 4 );
	memcpy( (char *) record + 17 + length, &pos, 4 );
	return(0);
}


RC RelationManager::getCatalogAttribute( vector<Attribute> &attrs1, vector<Attribute> &attrs2 )
{
	Attribute attr;
	attr.name	= "table-id";
	attr.type	= TypeInt;
	attr.length	= (AttrLength) 4;
	attrs1.push_back( attr );
	attrs2.push_back( attr );
	attr.name	= "table-name";
	attr.type	= TypeVarChar;
	attr.length	= (AttrLength) 50;
	attrs1.push_back( attr );
	attr.name = "file-name";
	attrs1.push_back( attr );
	attr.name = "column-name";
	attrs2.push_back( attr );
	attr.name	= "column-type";
	attr.type	= TypeInt;
	attr.length	= (AttrLength) 4;
	attrs2.push_back( attr );
	attr.name = "column-length";
	attrs2.push_back( attr );
	attr.name = "column-position";
	attrs2.push_back( attr );
	return(0);
}


RC RelationManager::createCatalog()
{
	RC	rc1	= RecordBasedFileManager::createFile( "Tables" );
	RC	rc2	= RecordBasedFileManager::createFile( "Columns" );
	if ( rc1 != 0 || rc2 != 0 )
	{
		return(-1);
	}

	/* Construct and insert the first two records of Tables */
	string tableName1, tableName2;
	tableName1	= "Tables";
	tableName2	= "Columns";
	vector<Attribute>	attrs1;
	vector<Attribute>	attrs2;

	getCatalogAttribute( attrs1, attrs2 );
	createTable( tableName1, attrs1 );
	createTable( tableName2, attrs2 );

	return(0);
}


RC RelationManager::deleteCatalog()
{
	RM_ScanIterator rmsi;
	vector<string>	attributes;
	attributes.push_back( "file-name" );
	scan( "Tables", "", NO_OP, NULL, attributes, rmsi );
	RID		rid;
	void		*returnedData = malloc( 4000 );
	vector<string>	tableNameAttr;
	while ( rmsi.getNextTuple( rid, returnedData ) != RM_EOF )
	{
		int size = *(int *) ( (char *) returnedData + 1);
		/* cout<<"size: "<<size<<endl; */
		char *buffer = (char *) malloc( size + 1 );
		memcpy( buffer, (char *) returnedData + 4 + 1, size );
		buffer[size] = 0;
		tableNameAttr.push_back( buffer );
		/* cout<<"tableName: "<<buffer<<endl; */
	}
	rmsi.close();
	free( returnedData );

	RC rc;
	for ( unsigned int i = 0; i < tableNameAttr.size(); i++ )
	{
		rc = RecordBasedFileManager::destroyFile( tableNameAttr[i] );
		if ( rc != 0 )
		{
			return(-1);
		}
	}
	return(0);
}


RC RelationManager::createTable( const string &tableName, const vector<Attribute> &attrs )
{
	void	*record = malloc( PAGE_SIZE );
	RID	rid;
	string	fileName	= tableName;
	string	tableName1	= "Tables";
	string	tableName2	= "Columns";

	/* Potential problems: when creating new file in the beginning, some catalog info will be written into this new file (don't know why) */
	if ( fileName != tableName1 && fileName != tableName2 )  /* Potential problems: how to avoid repetitively creation of tlb_Tables or tlb_Columns */
	{
		RC rc = RecordBasedFileManager::createFile( fileName );
		if ( rc != 0 )
		{
			return(-1);
		}
	}

	vector<Attribute>	attrs1;
	vector<Attribute>	attrs2;
	getCatalogAttribute( attrs1, attrs2 );

	FileHandle fileHandle1, fileHandle2;
	RecordBasedFileManager::openFile( tableName1, fileHandle1 );
	RecordBasedFileManager::openFile( tableName2, fileHandle2 );

	id = fileHandle1.id;
	id++;
	tablesRecordGenerator( id, tableName, fileName, record );
	RecordBasedFileManager::insertRecord( fileHandle1, attrs1, record, rid );
	memset( record, 0, PAGE_SIZE );
	for ( unsigned int i = 0; i < attrs.size(); i++ )
	{
		string	attrName	= attrs[i].name;
		int	attrType	= (int) attrs[i].type;
		int	attrLength	= (int) attrs[i].length;
		columnsRecordGenerator( id, attrName, attrType, attrLength, i, record );
		RecordBasedFileManager::insertRecord( fileHandle2, attrs2, record, rid );
		memset( record, 0, PAGE_SIZE );
	}
	fileHandle1.id	= id;
	fileHandle2.id	= id;
	RecordBasedFileManager::closeFile( fileHandle1 );
	RecordBasedFileManager::closeFile( fileHandle2 );
	free( record );

	return(0);
}


RC RelationManager::deleteTable( const string &tableName )
{
	FileHandle fileHandle;
	if ( RecordBasedFileManager::openFile( tableName, fileHandle ) != 0 )
	{
		return(-1);
	}
	string	fileName	= tableName;
	string	tableName1	= "Tables";
	string	tableName2	= "Columns";
	/* cannot delete catalog by deleteTable function */
	if ( fileName == tableName1 || fileName == tableName2 )
	{
		return(-1);
	}

	vector<Attribute>	attrs1;
	vector<Attribute>	attrs2;
	getCatalogAttribute( attrs1, attrs2 );

	/* get handlers of two files */
	FileHandle fileHandle1, fileHandle2;
	if ( RecordBasedFileManager::openFile( tableName1, fileHandle1 ) != 0
	     || RecordBasedFileManager::openFile( tableName2, fileHandle2 ) != 0 )
	{
		return(-1);
	}

	void *value = malloc( PAGE_SIZE );
	memset( value, 0, PAGE_SIZE );
	int len = tableName.length();
	memcpy( value, &len, sizeof(int) );
	memcpy( (char *) value + 4, tableName.c_str(), len );
	CompOp			compOp = EQ_OP;
	RBFM_ScanIterator	rbfm_ScanIterator;
	vector<string>		attributeNames;
	attributeNames.push_back( "table-id" );
	RecordBasedFileManager::scan( fileHandle1, attrs1, "table-name", compOp, value, attributeNames, rbfm_ScanIterator );
	RID	rid;
	void	*data = malloc( PAGE_SIZE );
	rbfm_ScanIterator.getNextRecord( rid, data );
	/* get table_id value */
	int idValue = *(int *) ( (char *) data + 1);
	memset( data, 0, PAGE_SIZE );
	memset( value, 0, PAGE_SIZE );
	memcpy( value, &idValue, 4 );
	/* delete register info in "Tables" */
	deleteTuple( "Tables", rid );

	attributeNames.clear();
	attributeNames.push_back( "column-name" );
	attributeNames.push_back( "column-type" );
	attributeNames.push_back( "column-length" );
	RBFM_ScanIterator rbfm_ScanIterator2;
	RecordBasedFileManager::scan( fileHandle2, attrs2, "table-id", compOp, value, attributeNames, rbfm_ScanIterator2 );

	while ( rbfm_ScanIterator2.getNextRecord( rid, data ) != -1 )
	{
		deleteTuple( "Columns", rid );
	}

	RecordBasedFileManager::closeFile( fileHandle );
	RecordBasedFileManager::closeFile( fileHandle1 );
	RecordBasedFileManager::closeFile( fileHandle2 );
	/* destroy the table file */
	RecordBasedFileManager::destroyFile( tableName );
	free( value );
	free( data );
	return(0);
}


RC RelationManager::getAttributes( const string &tableName, vector<Attribute> &attrs )
{
	string	tableName1	= "Tables";
	string	tableName2	= "Columns";

	vector<Attribute>	attrs1;
	vector<Attribute>	attrs2;
	getCatalogAttribute( attrs1, attrs2 );

	FileHandle fileHandle1, fileHandle2;
	RecordBasedFileManager::openFile( tableName1, fileHandle1 );
	RecordBasedFileManager::openFile( tableName2, fileHandle2 );

	CompOp	compOp		= EQ_OP;
	void	*value		= malloc( PAGE_SIZE );
	void	*pageReader	= malloc( PAGE_SIZE );
	memset( value, 0, PAGE_SIZE );
	int len = tableName.length();
	memcpy( value, &len, sizeof(int) );
	memcpy( (char *) value + 4, tableName.c_str(), len );
	vector<string> attributeNames;
	attributeNames.push_back( "table-id" );
	RBFM_ScanIterator rbfm_ScanIterator;
	RecordBasedFileManager::scan( fileHandle1, attrs1, "table-name", compOp, value, attributeNames, rbfm_ScanIterator );
	RID	rid;
	void	*data = malloc( PAGE_SIZE );
	rbfm_ScanIterator.getNextRecord( rid, data );
	int idValue = *(int *) ( (char *) data + 1);
	memset( data, 0, PAGE_SIZE );
	memset( value, 0, PAGE_SIZE );
	memcpy( value, &idValue, 4 );

	attributeNames.clear();
	attributeNames.push_back( "column-name" );
	attributeNames.push_back( "column-type" );
	attributeNames.push_back( "column-length" );
	RBFM_ScanIterator rbfm_ScanIterator2;
	RecordBasedFileManager::scan( fileHandle2, attrs2, "table-id", compOp, value, attributeNames, rbfm_ScanIterator2 );
	while ( rbfm_ScanIterator2.getNextRecord( rid, data ) != -1 )
	{
		Attribute	attr;
		string		s;
		char		p	= *(char *) data;
		int		offset	= 1;
		if ( (p & 0x80) != 0x80 )
		{
			int length = *(int *) ( (char *) data + 1);
			offset += sizeof(int);
			for ( int i = 0; i < length; i++ )
			{
				char ch = *( (char *) data + offset + i);
				s += ch;
			}
			offset += length;
		}
		attr.name = s;
		if ( (p & 0x40) != 0x40 )
		{
			attr.type	= (AttrType) (*(int *) ( (char *) data + offset) );
			offset		+= sizeof(int);
		}
		if ( (p & 0x20) != 0x20 )
		{
			attr.length	= (AttrLength) (*(int *) ( (char *) data + offset) );
			offset		+= sizeof(int);
		}
		attrs.push_back( attr );
	}
	RecordBasedFileManager::closeFile( fileHandle1 );
	RecordBasedFileManager::closeFile( fileHandle2 );
	free( value );
	free( data );
	free( pageReader );
	return(0);
}


RC RelationManager::insertTuple( const string &tableName, const void *data, RID &rid )
{
	FileHandle		fileHandle;
	vector<Attribute>	descriptor;
	if ( instance()->getAttributes( tableName, descriptor ) == 0 )
	{
		/* should search tables table (i.e. catalog) to search the file name related to this table (named tableName) */
		if ( RecordBasedFileManager::openFile( tableName, fileHandle ) == 0 )
		{
			RC	rc1	= RecordBasedFileManager::insertRecord( fileHandle, descriptor, data, rid );
			RC	rc2	= RecordBasedFileManager::closeFile( fileHandle );
			if ( rc1 != 0 || rc2 != 0 )
			{
				return(-1);
			}
			return(0);
		}
	}
	return(-1);
}


RC RelationManager::deleteTuple( const string &tableName, const RID &rid )
{
	FileHandle		fileHandle;
	vector<Attribute>	descriptor;
	if ( instance()->getAttributes( tableName, descriptor ) == 0 )
	{
		if ( RecordBasedFileManager::openFile( tableName, fileHandle ) == 0 )
		{
			RC	rc1	= RecordBasedFileManager::deleteRecord( fileHandle, descriptor, rid );
			RC	rc2	= RecordBasedFileManager::closeFile( fileHandle );
			if ( rc1 != 0 || rc2 != 0 )
			{
				return(-1);
			}
			return(0);
		}
	}
	return(-1);
}


RC RelationManager::updateTuple( const string &tableName, const void *data, const RID &rid )
{
	FileHandle		fileHandle;
	vector<Attribute>	descriptor;
	if ( instance()->getAttributes( tableName, descriptor ) == 0 )
	{
		if ( RecordBasedFileManager::openFile( tableName, fileHandle ) == 0 )
		{
			RC	rc1	= RecordBasedFileManager::updateRecord( fileHandle, descriptor, data, rid );
			RC	rc2	= RecordBasedFileManager::closeFile( fileHandle );
			if ( rc1 != 0 || rc2 != 0 )
			{
				return(-1);
			}
			return(0);
		}
	}
	return(-1);
}


RC RelationManager::readTuple( const string &tableName, const RID &rid, void *data )
{
	FileHandle		fileHandle;
	vector<Attribute>	descriptor;
	if ( instance()->getAttributes( tableName, descriptor ) == 0 )
	{
		if ( RecordBasedFileManager::openFile( tableName, fileHandle ) == 0 )
		{
			RC	rc1	= RecordBasedFileManager::readRecord( fileHandle, descriptor, rid, data );
			RC	rc2	= RecordBasedFileManager::closeFile( fileHandle );
			if ( rc1 != 0 || rc2 != 0 )
			{
				return(-1);
			}
			return(0);
		}
	}
	return(-1);
}


RC RelationManager::printTuple( const vector<Attribute> &attrs, const void *data )
{
	return(RecordBasedFileManager::printRecord( attrs, data ) );
}


RC RelationManager::readAttribute( const string &tableName, const RID &rid, const string &attributeName, void *data )
{
	FileHandle		fileHandle;
	vector<Attribute>	descriptor;
	if ( instance()->getAttributes( tableName, descriptor ) == 0 )
	{
		if ( RecordBasedFileManager::openFile( tableName, fileHandle ) == 0 )
		{
			RC	rc1	= RecordBasedFileManager::readAttribute( fileHandle, descriptor, rid, attributeName, data );
			RC	rc2	= RecordBasedFileManager::closeFile( fileHandle );
			if ( rc1 != 0 || rc2 != 0 )
			{
				return(-1);
			}
			return(0);
		}
	}
	return(-1);
}


RC RelationManager::scan( const string &tableName,
			  const string &conditionAttribute,
			  const CompOp compOp,
			  const void *value,
			  const vector<string> &attributeNames,
			  RM_ScanIterator &rm_ScanIterator )
{
	FileHandle		fileHandle;
	vector<Attribute>	descriptor;
	if ( tableName == "Tables" || tableName == "Columns" )
	{
		vector<Attribute> alternate;
		if ( tableName == "Tables" )
		{
			getCatalogAttribute( descriptor, alternate );
		} else if ( tableName == "Columns" )
		{
			getCatalogAttribute( alternate, descriptor );
		}
	} else {
		getAttributes( tableName, descriptor );
	}

	if ( RecordBasedFileManager::openFile( tableName, fileHandle ) == 0 )
	{
		if ( RecordBasedFileManager::scan( fileHandle, descriptor, conditionAttribute, compOp, value,
						   attributeNames, rm_ScanIterator.rbfm_ScanIterator ) == 0 )
		{
			return(0);
		}
	}
	return(-1);
}


/* Extra credit work */
RC RelationManager::dropAttribute( const string &tableName, const string &attributeName )
{
	return(-1);
}


/* Extra credit work */
RC RelationManager::addAttribute( const string &tableName, const Attribute &attr )
{
	return(-1);
}