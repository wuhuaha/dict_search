/*
 * friso lexicon implemented functions.
 *         used to deal with the friso lexicon, like: load,remove,match...
 * 
 * @author    chenxin <chenxin619315@gmail.com>
 */
#include <stdlib.h>
#include <string.h>
#include "friso_API.h"
#include "friso.h"

#define __SPLIT_MAX_TOKENS__     5
#define __LEX_FILE_DELIME__     '#'
#define __FRISO_LEX_IFILE__    "friso.lex.ini"

//create a new lexicon
FRISO_API friso_dic_t friso_dic_new() 
{
    register uint_t t;
    friso_dic_t dic = ( friso_dic_t ) FRISO_CALLOC( 
            sizeof( friso_hash_t ), __FRISO_LEXICON_LENGTH__ );
    if ( dic == NULL ) {
        ___ALLOCATION_ERROR___
    }

    for ( t = 0; t < __FRISO_LEXICON_LENGTH__; t++ ) {
        dic[t] = new_hash_table();
    }

    return dic;
}

/**
 * default callback function to invoke
 *     when free the friso dictionary . 
 *
 * @date 2013-06-12
 */
FRISO_API void default_fdic_callback( hash_entry_t e ) 
{
    register uint_t i;
    friso_array_t syn;
    lex_entry_t lex = ( lex_entry_t ) e->_val;
    //free the lex->word
    FRISO_FREE( lex->word ); 
    FRISO_FREE( lex->py );   
    FRISO_FREE( lex->lable );   
    //free the lex->syn if it is not NULL
    if ( lex->syn != NULL ) {
        syn = lex->syn;
        for ( i = 0; i < syn->length; i++ ) {
            FRISO_FREE( syn->items[i] );
        }
        free_array_list( syn );
    }

    //free the e->_val
    //@date 2014-01-28 posted by mlemay@gmail.com
    FRISO_FREE(lex);
}

FRISO_API void friso_dic_free( friso_dic_t dic ) 
{
    register uint_t t;
    for ( t = 0; t < __FRISO_LEXICON_LENGTH__; t++ ) {
        //free the hash table
        free_hash_table( dic[t], default_fdic_callback );
    }

    FRISO_FREE( dic );
}


//create a new lexicon entry
FRISO_API lex_entry_t new_lex_entry( 
        fstring word, 
        friso_array_t syn, 
        uint_t fre, 
        uint_t length, 
        uint_t type ) 
{
    lex_entry_t e = ( lex_entry_t ) 
        FRISO_MALLOC( sizeof( lex_entry_cdt ) );
    if ( e == NULL ) {
        ___ALLOCATION_ERROR___
    }

    //initialize.
    e->word   = word;
    e->syn    = syn;            //synoyum words array list.
    e->pos    = NULL;            //part of speech array list.
    e->py    = NULL; //set to NULL first.
    e->lable = NULL;
    e->fre    = fre;
    e->length = (uchar_t) length;    //length
    e->rlen   = (uchar_t) length;    //set to length by default.
    e->type   = (uchar_t) type;    //type
    e->ctrlMask = 0;            //control mask.
    e->offset = -1;

    return e;
}

FRISO_API key_rex_entry_t new_key_rex_entry( 
				fstring word, 
				fstring pinyin, 
				fstring label) 
{
	key_rex_entry_t e = ( key_rex_entry_t )
			FRISO_MALLOC( sizeof( key_rex_entry ));
	if( e == NULL ){
		___ALLOCATION_ERROR___
	}
	e->word = word;
	e->pinyin = pinyin;
	e->lable = label;
	e->word_list = NULL;
	e->py_list = NULL;

	return e;
}



/**
 * free the given lexicon entry.
 * you have to do three thing maybe:
 * 1. free where its syn items points to. (not implemented)
 * 2. free its syn. (friso_array_t)
 * 3. free its pos. (friso_array_t)
 * 4. free the lex_entry_t.
 */
FRISO_API void free_lex_entry_full( lex_entry_t e ) 
{
    register uint_t i;
    friso_array_t syn;

    //free the lex->word
    FRISO_FREE( e->word );
    FRISO_FREE( e->py );
    FRISO_FREE( e->lable );
    //free the lex->syn if it is not NULL
    if ( e->syn != NULL ) {
        syn = e->syn;
        for ( i = 0; i < syn->length; i++ ) {
            FRISO_FREE( syn->items[i] );
        }
        free_array_list( syn );
    }

    //free the e->_val
    //@date 2014-01-28 posted by mlemay@gmail.com
    FRISO_FREE(e);
}

FRISO_API void free_lex_entry( lex_entry_t e ) 
{
    //if ( e->syn != NULL ) {
    //    if ( flag == 1 ) free_array_list( e->syn);
    //    else free_array_list( e->syn );
    //}

    FRISO_FREE(e);
}


//add a new entry to the dictionary.
FRISO_API void friso_dic_add( 
        friso_dic_t dic, 
        friso_lex_t lex,
        fstring word, 
        friso_array_t syn ) 
{
    void *olex = NULL;
    lex_entry_t old_entry = NULL;
	int  fre = 0;
    if ( lex >= 0 && lex < __FRISO_LEXICON_LENGTH__ ) {
        //printf("lex=%d, word=%s, syn=%s\n", lex, word, syn);
        if(old_entry != NULL){
			if(old_entry->fre){
				fre = old_entry->fre;
			    printf("old-entry:fre:%d\n", old_entry->fre);
			}
		}
        olex = hash_put_mapping( dic[lex], word, 
                new_lex_entry( word, syn, fre, 
                    (uint_t) strlen(word),  (uint_t) lex ) );
        if ( olex != NULL ) {
            free_lex_entry_full((lex_entry_t)olex);
        }
    }
}

FRISO_API void friso_dic_add_with_fre( 
        friso_dic_t dic,
        friso_lex_t lex,
        fstring word, 
        friso_array_t syn, 
        uint_t frequency ) 
{
    void *olex = NULL;    
	lex_entry_t old_entry = NULL;
    if ( lex >= 0 && lex < __FRISO_LEXICON_LENGTH__ ) {
        old_entry = (lex_entry_t)hash_get_value(dic[lex], word);
		if(old_entry != NULL){
			if((!old_entry->fre)&&(frequency)){
				old_entry->fre = frequency;
                //printf("add fre:%d\n", old_entry->fre);
			}
		}else{
        	olex = hash_put_mapping( dic[lex], word, 
               	 new_lex_entry( word, syn, frequency, 
                    	( uint_t ) strlen(word), ( uint_t ) lex ) );
        	if ( olex != NULL ) {
            	free_lex_entry_full((lex_entry_t)olex);
        	}
		}
    }
}

//添加拼音
FRISO_API void friso_dic_add_pinyin( 
        friso_dic_t dic,
        friso_lex_t lex,
        fstring word, 
        fstring pinyin ) 
{   
	lex_entry_t old_entry = NULL;
    if(pinyin == NULL) return;
    if ( lex >= 0 && lex < __FRISO_LEXICON_LENGTH__ ) {
        old_entry = (lex_entry_t)hash_get_value(dic[lex], word);
		if(old_entry != NULL){
			if(old_entry->py == NULL){
				old_entry->py = pinyin;
                //printf("add pinyin:%s to word:%s\n", old_entry->py, old_entry->word);
			}
		}else{
        	printf("None this entry,so,did't add pinyin");
		}
    }
}

//添加标签
FRISO_API lex_entry_t friso_dic_add_lable( 
        friso_dic_t dic,
        friso_lex_t lex,
        fstring word, 
        fstring lable ) 
{   
	lex_entry_t old_entry = NULL;
    if(lable == NULL) return NULL;
    if ( lex >= 0 && lex < __FRISO_LEXICON_LENGTH__ ) {
        old_entry = (lex_entry_t)hash_get_value(dic[lex], word);
		if(old_entry != NULL){
			if(old_entry->lable == NULL){
				old_entry->lable = lable;
                //printf("add lable:%s to word:%s\n", old_entry->lable, old_entry->word);
			}
			return old_entry;
		}else{
        	printf("None this entry,so,did't add lable");
		}
    }
	return NULL;
}
/*
 * read a line from a specified stream.
 *         the newline will be cleared.
 * 
 * @date    2012-11-24 
 */
FRISO_API fstring file_get_line( fstring __dst, FILE * _stream ) 
{
    register int c;
    fstring cs;

    cs = __dst;
    while ( ( c = fgetc( _stream ) ) != EOF ) {
        if ( c == '\n' ) break;
        *cs++ = c; 
    }
    *cs = '\0';

    return ( c == EOF && cs == __dst ) ? NULL : __dst;
}

/*
 * static function to copy a string. 
 */
///instead of memcpy
__STATIC_API__ fstring string_copy( 
        fstring _src, 
        fstring __dst, 
        uint_t blocks ) 
{

    register fstring __src = _src;
    register uint_t t;

    for ( t = 0; t < blocks; t++ ) {
        if ( *__src == '\0' ) break; 
        __dst[t] = *__src++;
    }
    __dst[t] = '\0';

    return __dst;
}

/**
 * make a heap allocation, and copy the 
 *     source fstring to the new allocation, and 
 *     you should free it after use it . 
 *
 * @param _src      source fstring
 * @param blocks    number of bytes to copy
 */
FRISO_API fstring string_copy_heap( 
        fstring _src, uint_t blocks ) 
{
    register uint_t t;

    fstring str = ( fstring ) FRISO_MALLOC( blocks + 1 );
    if ( str == NULL ) {
        ___ALLOCATION_ERROR___;
    }

    for ( t = 0; t < blocks; t++ ) {
        //if ( *_src == '\0' ) break;
        str[t] = *_src++;
    }

    str[t] = '\0';
    return str;
}

/*
 * find the postion of the first appear of the given char.
 *    address of the char in the fstring will be return .
 *    if not found NULL will be return . 
 */
__STATIC_API__ fstring indexOf( fstring __str, char delimiter ) 
{
    uint_t i, __length__;

    __length__ = strlen( __str );
    for ( i = 0; i < __length__; i++ ) {
        if ( __str[i] == delimiter ) {
            return __str + i;
        }
    }

    return NULL;
}

/**
 * load all the valid wors from a specified lexicon file . 
 *
 * @param dic        friso dictionary instance (A hash array)
 * @param lex        the lexicon type
 * @param lex_file    the path of the lexicon file
 * @param length    the maximum length of the word item
 */
FRISO_API void friso_dic_load( 
        friso_t friso,
        friso_config_t config,
        friso_lex_t lex,
        fstring lex_file,
        uint_t length ) 
{

    FILE * _stream;
    char __char[1024], _buffer[512];
    fstring _line;
    string_split_entry sse;

    fstring _word;
    char _sbuffer[512];
    char _pbuffer[512];
    fstring _syn;
	fstring _pinyin;
    fstring _lable;
    friso_array_t sywords;
    uint_t _fre;

    if ( ( _stream = fopen( lex_file, "rb" ) ) != NULL ) {
        while ( ( _line = file_get_line( __char, _stream ) ) != NULL ) {
            //clear up the notes
            //make sure the length of the line is greater than 1.
            //like the single '#' mark in stopwords dictionary.
            if ( _line[0] == '#' && strlen(_line) > 1 ) continue;

            //handle the stopwords.
            if ( lex == __LEX_STOPWORDS__ ) {
                //clean the chinese words that its length is greater than max length.
                if ( ((int)_line[0]) < 0 && strlen( _line ) > length ) continue;
                friso_dic_add( friso->dic, __LEX_STOPWORDS__, 
                        string_copy_heap( _line, strlen(_line) ), NULL ); 
                continue;
            }

            //split the fstring with '/'.
            string_split_reset( &sse, "/", _line); 
            if ( string_split_next( &sse, _buffer ) == NULL ) {
                continue;
            }

            //1. get the word.
            _word = string_copy_heap( _buffer, strlen(_buffer) );

            if ( string_split_next( &sse, _buffer ) == NULL ) {
                //normal lexicon type, 
                //add them to the dictionary directly
                friso_dic_add( friso->dic, lex, _word, NULL ); 
                continue;
            }

            /*
             * filter out the words that its length is larger
             *     than the specified limit.
             * but not for __LEX_ECM_WORDS__ and english __LEX_STOPWORDS__
             *     and __LEX_CEM_WORDS__.
             */
            if ( ! ( lex == __LEX_ECM_WORDS__ || lex == __LEX_CEM_WORDS__ )
                    && strlen( _word ) > length ) {
                FRISO_FREE(_word);
                continue;
            }

            //2. get the synonyms words.
            _syn = NULL;
            if ( strcmp( _buffer, "null" ) != 0 ) {
                _syn = string_copy( _buffer, _sbuffer, strlen(_buffer) );
            }

            //3. get the word frequency if it available.
            _fre = 0;
            if ( string_split_next( &sse, _buffer ) != NULL ) {
                _fre = atoi( _buffer );
            }

			//4. get the word pinyin if it available.
            _pinyin = NULL;
            if ( string_split_next( &sse, _buffer ) != NULL ) {
                _pinyin = string_copy( _buffer, _pbuffer, strlen(_buffer) );
            }

			//5. get the word lable if it available.
            _lable = NULL;
            if ( string_split_next( &sse, _buffer ) != NULL ) {
                _lable = string_copy_heap( _buffer, strlen(_buffer) );
            }
            /**
             * Here:
             * split the synonyms words with mark "," 
             *     and put them in a array list if the synonyms is not NULL
             */
            sywords = NULL;
            if ( config->add_syn && _syn != NULL ) {
                string_split_reset( &sse, "|", _sbuffer );
                sywords = new_array_list_with_opacity(5);
                while ( string_split_next( &sse, _buffer ) != NULL ) {
                    if ( strlen(_buffer) > length ) continue;
                    array_list_add( sywords, 
                            string_copy_heap(_buffer, strlen(_buffer)) );
                }
                sywords = array_list_trim( sywords );
            }
            
			//单个字转拼音，只取最常用的一个
			if ( (_pinyin != NULL) && (strcmp(_pinyin, "null") != 0) ) {
				if(get_utf8_bytes(_word[0]) == strlen(_word)){
                	string_split_reset( &sse, ",", _pbuffer );
					_pinyin = NULL;
					if ( string_split_next( &sse, _pbuffer ) != NULL ) {
                       _pinyin = string_copy_heap( _pbuffer, strlen(_pbuffer) );
            		}
                }else{
                       _pinyin = string_copy_heap( _pbuffer, strlen(_pbuffer) );
                }
				//printf("pinyin of %s is %s\n", _word, _pinyin);
            }

            //4. add the word item
            friso_dic_add_with_fre( 
                    friso->dic, lex, _word, sywords, _fre );
            //add pinyin    
            if( (_pinyin != NULL) && (strcmp(_pinyin, "null") != 0) ) {
                friso_dic_add_pinyin( 
                    friso->dic, lex, _word, _pinyin);
            }
             //add lable    
           if( (_lable != NULL) && (strcmp(_lable, "null") != 0) ){
                friso_dic_add_lable( 
                    friso->dic, lex, _word, _lable);
            }
        } 

        fclose( _stream );
    } else {
        fprintf(stderr, "Warning: Fail to open lexicon file %s\n", lex_file);
        fprintf(stderr, "Warning: Without lexicon file, segment results will not correct \n");
    } 
}

/**
 * load all the valid wors from a specified sql . 
 *
 * @param dic        friso dictionary instance (A hash array)
 * @param lex        the lexicon type
 * @param lex_table    the name of the lexicon table
 * @param length    the maximum length of the word item
 */
FRISO_API void friso_dic_load_by_sql( 
        friso_t friso,
        friso_config_t config,
        friso_lex_t lex,
        fstring lex_table,
        uint_t length ) 
{

    MYSQL * sql = config->mysql;
	MYSQL_RES * mysqlResult = NULL;
	MYSQL_ROW mysqlRow;
	fstring _word;
    fstring _syn;
	fstring _pinyin;
    fstring _lable;
	fstring tmp;
    friso_array_t sywords;
    uint_t _fre;
	int ret = 0;
	char sql_query[512];
    char _buffer[512];
    char _pbuffer[512];
	register int iNumRow = 0;
	string_split_entry sse;
	int flag = 0;
	lex_entry_t  lex_entry;
	key_rex_entry_t  key_rex;

	if(strstr(lex_table, "domain_") == lex_table){
		flag = 1;
	}
	
	snprintf(sql_query, sizeof(sql_query),"SELECT word, syn, fre, pinyin, label FROM %s", lex_table);
	if ((ret = mysql_query(sql, sql_query)))
	{
		printf("sql select error!error code:%d\n", ret);
		return ;
  	}
	
	mysqlResult = mysql_store_result(sql);
	
	if (mysqlResult == NULL)
	{
		printf("sql select error!error code: %d:%s\n", mysql_errno(sql), mysql_error(sql));
		return ;
	}
 
	iNumRow = mysql_num_rows(mysqlResult);
 
	printf("there are %d records in %s\n", iNumRow, lex_table);
	 
	while ((mysqlRow = mysql_fetch_row(mysqlResult)))
	{
		//word
		lex_entry = NULL;
		key_rex = NULL;
		_word = string_copy_heap( mysqlRow[0], strlen(mysqlRow[0]));
		//friso_dic_add( friso->dic, lex, _word, NULL ); 
		//syn
		if(mysqlRow[1] != NULL){
			_syn = string_copy_heap( mysqlRow[1], strlen(mysqlRow[1]));
		}else{
			_syn = NULL;
		}
		//fre
		if(mysqlRow[2] != NULL){
			_fre = atoi(mysqlRow[2]);
		}else{
			_fre = 0;
		}
		//pinyin,单个字的读音只取第一个
		if(mysqlRow[3] != NULL){
			if(get_utf8_bytes(_word[0]) == strlen(_word)){
                string_split_reset( &sse, ",", mysqlRow[3] );
				_pinyin = NULL;
				if ( string_split_next( &sse, _pbuffer ) != NULL ) {
               		_pinyin = string_copy_heap( _pbuffer, strlen(_pbuffer) );
            	}else{
            		_pinyin = string_copy_heap( mysqlRow[3], strlen(mysqlRow[3]));
            	}
            }else{
               _pinyin = string_copy_heap( mysqlRow[3], strlen(mysqlRow[3]));
            }
		}else{
			_pinyin = NULL;
		}
		//label	
		if(mysqlRow[4] != NULL){
			_lable = string_copy_heap( mysqlRow[4], strlen(mysqlRow[4]));
		}else{
			_lable = NULL;
		}

		/**
         * Here:
         * split the synonyms words with mark "|" 
         *     and put them in a array list if the synonyms is not NULL
         */
         sywords = NULL;
         if ( config->add_syn && _syn != NULL ) {
            string_split_reset( &sse, "|", _syn );
            sywords = new_array_list_with_opacity(5);
            while ( string_split_next( &sse, _buffer ) != NULL ) {
               if ( strlen(_buffer) > length ) continue;
                    array_list_add( sywords, 
                            string_copy_heap(_buffer, strlen(_buffer)) );
                }
           sywords = array_list_trim( sywords );
         }           		

		if((strchr(_word, ' ') == NULL) && (strchr(_word, ' ') == NULL) && (strchr(_word, '*') == NULL) && (strchr(_word, '*') == NULL)){

			//add the word item
      		friso_dic_add_with_fre( 
       			friso->dic, lex, _word, sywords, _fre );
      		//add pinyin    
      		if( (_pinyin != NULL) && (strcmp(_pinyin, "null") != 0) ) {
      			friso_dic_add_pinyin( 
                    friso->dic, lex, _word, _pinyin);
       		}
        	//add lable    
      		if( (_lable != NULL) && (strcmp(_lable, "null") != 0) ){
            	lex_entry = friso_dic_add_lable( 
                    friso->dic, lex, _word, _lable);
      		}

			if(flag == 1){
				array_list_add(friso->domain_pinyin, lex_entry);
				printf("add [%s][%s][%s] to pinyin list %s\n",_word, _pinyin, _lable, friso->domain);
			}
		
		}else{
			if(flag == 1){
				//printf("add [%s][%s][%s] to lex list %s\n",_word, _pinyin, _lable, friso->domain);
				key_rex= new_key_rex_entry(_word, _pinyin, _lable);
				key_rex->word_list = new_array_list_with_opacity(2);
        		key_rex->py_list = new_array_list_with_opacity(2);
				
        		string_split_reset( &sse, "*", key_rex->word);
        		while(string_split_next( &sse, _buffer ) != NULL)
       	 		{
            		tmp = string_copy_heap(_buffer, strlen(_buffer));
            		array_list_add(key_rex->word_list, tmp);
            		//log_debug(sa_log, "main", "%s",buffer);
        		} 
        		string_split_reset( &sse, ",*,", key_rex->pinyin);
        		while(string_split_next( &sse, _buffer ) != NULL)
        		{
            		tmp = string_copy_heap(_buffer, strlen(_buffer));
            		array_list_add(key_rex->py_list, tmp);
            		//log_debug(sa_log, "main", "%s",buffer);
        		} 
				array_list_add(friso->domain_rex, key_rex);
				printf("added [%s][%s][%s] to lex list %s\n",_word, _pinyin, _lable, friso->domain);
			}
		}

	}
	mysql_free_result(mysqlResult);
	return;

}


/**
 * get the lexicon type index with the specified 
 *     type keywords . 
 *
 * @see        friso.h#friso_lex_t
 * @param     _key
 * @return     int
 */
__STATIC_API__ friso_lex_t get_lexicon_type_with_constant( fstring _key ) 
{
    if ( strcmp( _key, "__LEX_CJK_WORDS__" ) == 0 ) {
        return __LEX_CJK_WORDS__;
    } else if ( strcmp( _key, "__LEX_CJK_UNITS__" ) == 0 ) {
        return __LEX_CJK_UNITS__;
    } else if ( strcmp( _key, "__LEX_ECM_WORDS__" ) == 0 ) {
        return __LEX_ECM_WORDS__;
    } else if ( strcmp( _key, "__LEX_CEM_WORDS__" ) == 0 ) {
        return __LEX_CEM_WORDS__;
    } else if ( strcmp( _key, "__LEX_CN_LNAME__" ) == 0 ) {
        return __LEX_CN_LNAME__;
    } else if ( strcmp( _key, "__LEX_CN_SNAME__" ) == 0 ) {
        return __LEX_CN_SNAME__;
    } else if ( strcmp( _key, "__LEX_CN_DNAME1__" ) == 0 ) {
        return __LEX_CN_DNAME1__;
    } else if ( strcmp( _key, "__LEX_CN_DNAME2__" ) == 0 ) {
        return __LEX_CN_DNAME2__;
    } else if ( strcmp( _key, "__LEX_CN_LNA__" ) == 0 ) {
        return __LEX_CN_LNA__;
    } else if ( strcmp( _key, "__LEX_STOPWORDS__" ) == 0 ) {
        return __LEX_STOPWORDS__;
    } else if ( strcmp( _key, "__LEX_ENPUN_WORDS__" ) == 0 ) {
        return __LEX_ENPUN_WORDS__;
    } else if ( strcmp( _key, "__LEX_EN_WORDS__" ) == 0 ) {
        return __LEX_EN_WORDS__;
    }

    return -1;
}

/*
 * load the lexicon configuration file.
 *        and load all the valid lexicon from the configuration file.
 *
 * @param friso        friso instance
 * @param    config    friso_config instance
 * @param _path        dictionary directory
 * @param _limitts    words length limit    
 */
FRISO_API void friso_dic_load_from_ifile( 
        friso_t friso, 
        friso_config_t config,
        fstring _path,
        uint_t _limits  ) 
{

    //1.parse the configuration file.
    FILE *__stream;
    char __chars__[1024], __key__[30], *__line__;
    uint_t __length__, i, t;
    friso_lex_t lex_t;
    string_buffer_t sb;

    //get the lexicon configruation file path
    sb = new_string_buffer();

    string_buffer_append( sb, _path );
    string_buffer_append( sb, __FRISO_LEX_IFILE__ );
    //printf("%s\n", sb->buffer);

    if ( ( __stream = fopen( sb->buffer, "rb" ) ) != NULL ) {
        while ( ( __line__ = 
                    file_get_line( __chars__, __stream ) ) != NULL ) {
            //comment filter.
            if ( __line__[0] == '#' )  continue;
            if ( __line__[0] == '\0' ) continue;

            __length__ = strlen( __line__ );
            //item start
            if ( __line__[ __length__ - 1 ] == '[' ) {
                //get the type key
                for ( i = 0; i < __length__
                        && ( __line__[i] == ' ' || __line__[i] == '\t' ); i++ ); 
                for ( t = 0; i < __length__; i++,t++ ) {
                    if ( __line__[i] == ' ' 
                            || __line__[i] == '\t' || __line__[i] == ':' ) break;
                    __key__[t] = __line__[i];
                }
                __key__[t] = '\0';

                //get the lexicon type
                lex_t = get_lexicon_type_with_constant(__key__);
                if ( lex_t == -1 ) continue; 

                //printf("key=%s, type=%d\n", __key__, lex_t );
                while ( ( __line__ = file_get_line( __chars__, __stream ) ) != NULL ) {
                    //comments filter.
                    if ( __line__[0] == '#' ) continue;
                    if ( __line__[0] == '\0' ) continue; 

                    __length__ = strlen( __line__ );
                    if ( __line__[ __length__ - 1 ] == ']' ) break;

                    for ( i = 0; i < __length__ 
                            && ( __line__[i] == ' ' || __line__[i] == '\t' ); i++ );
                    for ( t = 0; i < __length__; i++,t++ ) {
                        if ( __line__[i] == ' ' 
                                || __line__[i] == '\t' || __line__[i] == ';' ) break;
                        __key__[t] = __line__[i]; 
                    }
                    __key__[t] = '\0';

                    //load the lexicon item from the lexicon file.
                    string_buffer_clear( sb );
                    string_buffer_append( sb, _path );
                    string_buffer_append( sb, __key__ );
                    //printf("key=%s, type=%d\n", __key__, lex_t);
                    friso_dic_load( friso, config, lex_t, sb->buffer, _limits );
                }

            } 

        } //end while

        fclose( __stream );
    } else {
        fprintf(stderr, "Warning: Fail to open the lexicon configuration file %s\n", sb->buffer);
        fprintf(stderr, "Warning: Without lexicon file, segment results will not correct \n");
    }

    free_string_buffer(sb);    
}

//match the item.
FRISO_API int friso_dic_match( 
        friso_dic_t dic, 
        friso_lex_t lex, 
        fstring word ) 
{
    if ( lex >= 0 && lex < __FRISO_LEXICON_LENGTH__ ) {
        return hash_exist_mapping( dic[lex], word );
    }
    return 0;
}

//get the lex_entry_t associated with the word.
FRISO_API lex_entry_t friso_dic_get( 
        friso_dic_t dic, 
        friso_lex_t lex, 
        fstring word ) 
{
    if ( lex >= 0 && lex < __FRISO_LEXICON_LENGTH__ ) {
        return ( lex_entry_t ) hash_get_value( dic[lex], word );
    }
    return NULL;
}

//get the size of the specified type dictionary.
FRISO_API uint_t friso_spec_dic_size( 
        friso_dic_t dic, 
        friso_lex_t lex ) 
{
    if ( lex >= 0 && lex < __FRISO_LEXICON_LENGTH__ ) {
        return hash_get_size( dic[lex] );
    }
    return 0;
}

//get size of the whole dictionary.
FRISO_API uint_t friso_all_dic_size( 
        friso_dic_t dic ) 
{
    register uint_t size = 0, t;

    for ( t = 0; t < __FRISO_LEXICON_LENGTH__; t++ ) {
        size += hash_get_size( dic[t] );
    }

    return size;
}

//通过单个字单个字的方式获取词的拼音
FRISO_API fstring pinyin_single_get( 
		friso_dic_t dic, 
        friso_lex_t lex, 
        fstring word,
        fstring py) 
{
	lex_entry_t tmp;
	char key[__HITS_PINYIN_LENGTH__] = {0};
	register uint_t bytes = 0, idex = 0, length = 0;

	length = strlen(word);
    //printf("word:%s,length:%d",word,length);
	
	if ( lex >= 0 && lex < __FRISO_LEXICON_LENGTH__ ) {
        
		while((bytes = get_utf8_bytes(word[idex]))&&(idex < length)){
			memcpy(key, word + idex, bytes);
			key[bytes] = '\0';
        	tmp =  ( lex_entry_t ) hash_get_value( dic[lex], key );
            if(idex) strcat(py, ",");
			strcat(py,tmp->py);
			//printf("single word:%s,pinyin:%s,now py:%s\n",key, tmp->py, py);
			idex += bytes;
                       
		}
        
    }
   
    return NULL;		
}			
