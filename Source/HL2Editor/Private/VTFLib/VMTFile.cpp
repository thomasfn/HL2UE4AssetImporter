/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "VMTFile.h"
#include "VTFLib.h"

using namespace VTFLib;
using namespace VTFLib::Nodes;

CVMTFile::CVMTFile()
{
	this->Root = 0;
}

CVMTFile::CVMTFile(const CVMTFile &VMTFile)
{
	if(VMTFile.Root == 0)
	{
		this->Root = 0;
	}
	else
	{
		this->Root = new CVMTGroupNode(*VMTFile.Root);
	}
}

CVMTFile::~CVMTFile()
{
	delete this->Root;
}

bool CVMTFile::Create(const char *cRoot)
{
	delete this->Root;
	this->Root = new CVMTGroupNode(cRoot);

	return true;
}

void CVMTFile::Destroy()
{
	delete this->Root;
	this->Root = 0;
}

bool CVMTFile::IsLoaded() const
{
	return this->Root != 0;
}

bool CVMTFile::Load(const void *lpData, unsigned int uiBufferSize)
{
	IO::Readers::CMemoryReader reader(lpData, uiBufferSize);
	return this->Load(&reader);
}

bool CVMTFile::Load(void *pUserData)
{
	IO::Readers::CProcReader reader(pUserData);
	return this->Load(&reader);
}

bool CVMTFile::Save(void *lpData, unsigned int uiBufferSize, unsigned int &uiSize) const
{
	uiSize = 0;

	IO::Writers::CMemoryWriter MemoryWriter = IO::Writers::CMemoryWriter(lpData, uiBufferSize);

	bool bResult = this->Save(&MemoryWriter);

	uiSize = MemoryWriter.GetStreamSize();

	return bResult;
}

bool CVMTFile::Save(void *pUserData) const
{
	IO::Writers::CProcWriter writer(pUserData);
	return this->Save(&writer);
}

enum EToken
{
	TOKEN_EOF = 0,			// No more tokens to read.
	TOKEN_NEWLINE,			// Token is a newline (\n).
	TOKEN_WHITESPACE,		// Token is any whitespace other than a newline.
	TOKEN_FORWARD_SLASH,	// Token is a forward slash (/).
	TOKEN_QUOTE,			// Token is a quote (").
	TOKEN_OPEN_BRACE,		// Token is an open brace ({).
	TOKEN_CLOSE_BRACE,		// Token is a close brace (}).
	TOKEN_CHAR,				// Token is a char (any char).  Use GetChar().
	TOKEN_STRING,			// Token is a string.  Use GetString().
	TOKEN_QUOTED_STRING,	
	TOKEN_SPECIAL			// Token is a specified special char.
};

// Stores token information.
class CToken
{
private:
	EToken eToken;
	char cChar;
	char *lpString;

public:
	// Create a normal token.  cChar was the tokenized char.
	CToken(EToken eToken, char cChar = '\0') : eToken(eToken), cChar(cChar), lpString(0)
	{
		check(eToken != TOKEN_CHAR && eToken != TOKEN_STRING && eToken != TOKEN_QUOTED_STRING);
	}

	// Create a char token.
	CToken(char cChar) : eToken(TOKEN_CHAR), cChar(cChar), lpString(0)
	{

	}

	// Create a string token.
	CToken(char *lpString, bool bQuoted) : eToken(bQuoted ? TOKEN_QUOTED_STRING : TOKEN_STRING), cChar('\0'), lpString(0)
	{
		this->lpString = new char[strlen(lpString) + 1];
		strcpy_s(this->lpString, strlen(lpString) + 1, lpString);
	}

	// Copy a token.
	CToken(const CToken &Token)
	{
		this->eToken = Token.eToken;
		this->cChar = Token.cChar;
		this->lpString = 0;

		if(Token.lpString != 0)
		{
			this->lpString = new char[strlen(Token.lpString) + 1];
			strcpy_s(this->lpString, strlen(lpString) + 1, Token.lpString);
		}
	}

	~CToken()
	{
		delete []this->lpString;
	}

public:
	// Convert the current token to a special token.
	// We need to do this because the tokenizer reads ahead and doen't
	// know if the requested token will be special until after the fact.
	void ToSpecial(const char *lpSpecial)
	{
		if(this->eToken == TOKEN_EOF)
		{
			return;
		}

		for(char *pSpecial = const_cast<char *>(lpSpecial); *pSpecial != '\0'; pSpecial++)
		{
			if(this->cChar == *pSpecial)
			{
				this->eToken = TOKEN_SPECIAL;
				return;
			}
		}

		this->eToken = TOKEN_CHAR;
	}

	// Get the token that was read.
	EToken GetToken() const
	{
		return this->eToken;
	}

	// Get the char that was tokenized.  Only works if
	// token is a TOKEN_CHAR or was tokenized by the byte
	// tokenizer.
	const char GetChar() const
	{
		return this->cChar;
	}

	// Get the string that was tokenized.  Only works if
	// token is a TOKEN_STRING.
	const char *GetString() const
	{
		return this->lpString;
	}
};

// Tokenizes single byte tokens.
class CByteTokenizer
{
private:
	unsigned int uiLine;
	IO::Readers::IReader *Reader;

private:
	CToken *CurrentToken;
	CToken *NextToken;

public:
	CByteTokenizer(IO::Readers::IReader *Reader) : uiLine(1), Reader(Reader), CurrentToken(0), NextToken(0)
	{
		this->GetNextToken();
	}

	~CByteTokenizer()
	{
		delete this->CurrentToken;
		delete this->NextToken;
	}

private:
	void GetNextToken(const char *lpSpecial = 0)
	{
		char cChar;

		if(!Reader->Read(cChar))
		{
			this->NextToken = new CToken(TOKEN_EOF);
			return;
		}

		// Keep track of the line number.
		if(cChar == '\n')
		{
			this->uiLine++;
		}

		// If a special char was specified, only return TOKEN_CHAR tokens
		// unless the special char was found in which case return a
		// TOKEN_SPECIAL.
		if(lpSpecial)
		{
			for(char *pSpecial = const_cast<char *>(lpSpecial); *pSpecial != '\0'; pSpecial++)
			{
				if(cChar == *pSpecial)
				{
					this->NextToken = new CToken(TOKEN_SPECIAL, cChar);
					return;
				}
			}

			this->NextToken = new CToken(cChar);
			return;
		}
		
		if(cChar == '\r' || cChar == '\n')
		{
			this->NextToken = new CToken(TOKEN_NEWLINE, cChar);
		}
		else if(isspace(cChar))
		{
			this->NextToken = new CToken(TOKEN_WHITESPACE, cChar);
		}
		else if(cChar == '/')
		{
			this->NextToken = new CToken(TOKEN_FORWARD_SLASH, cChar);
		}
		else if(cChar == '\"')
		{
			this->NextToken = new CToken(TOKEN_QUOTE, cChar);
		}
		else if(cChar == '{')
		{
			this->NextToken = new CToken(TOKEN_OPEN_BRACE, cChar);
		}
		else if(cChar == '}')
		{
			this->NextToken = new CToken(TOKEN_CLOSE_BRACE, cChar);
		}
		else
		{
			this->NextToken = new CToken(cChar);
		}
	}

public:
	// Get the current token and return the next one.
	CToken *Next(const char *lpSpecial = 0)
	{
		delete this->CurrentToken;
		this->CurrentToken = this->NextToken;
		this->NextToken = 0;

		if(lpSpecial && this->CurrentToken)
		{
			this->CurrentToken->ToSpecial(lpSpecial);
		}

		this->GetNextToken();

		return this->CurrentToken;
	}

	// Get the curret token.
	CToken *Peek()
	{
		return this->NextToken;
	}

	// Get the current line.
	unsigned int GetLine()
	{
		return this->uiLine;
	}
};

// Tokenizes multi byte tokens.
class CTokenizer
{
private:
	CByteTokenizer *ByteTokenizer;

private:
	CToken *CurrentToken;
	CToken *NextToken;

public:
	CTokenizer(CByteTokenizer *ByteTokenizer) : ByteTokenizer(ByteTokenizer), CurrentToken(0), NextToken(0)
	{
		this->GetNextToken();
	}

	~CTokenizer()
	{
		delete this->CurrentToken;
		delete this->NextToken;
	}

public:
	void GetNextToken()
	{
		CToken *Token;
		
		Token = this->ByteTokenizer->Next();

		// Consume all whitespace.
		while(Token->GetToken() == TOKEN_WHITESPACE)
		{
			Token = this->ByteTokenizer->Next();
		}

		unsigned int uiIndex = 0;
		char cBuffer[4096];

		switch(Token->GetToken())
		{
		// Comment (these are removed for the parser).
		case TOKEN_FORWARD_SLASH:
			Token = this->ByteTokenizer->Next();

			if(Token->GetToken() != TOKEN_FORWARD_SLASH)
			{
				throw "expected comment string";
			}

			do
			{
				Token = this->ByteTokenizer->Next("\n");
			} while(Token->GetToken() == TOKEN_CHAR);

			if(Token->GetToken() == TOKEN_EOF)
			{
				this->NextToken = new CToken(TOKEN_EOF);
			}
			else
			{
				this->NextToken = new CToken(TOKEN_NEWLINE);
			}
			break;
		// Quoted string.
		case TOKEN_QUOTE:
			while(true)
			{
				Token = this->ByteTokenizer->Next("\"");

				if(Token->GetToken() != TOKEN_CHAR)
				{
					break;
				}

				if(Token->GetChar() == '\r' || Token->GetChar() == '\n')
				{
					throw "newline in string";
				}

				cBuffer[uiIndex++] = Token->GetChar();
			}
			cBuffer[uiIndex++] = '\0';

			if(Token->GetToken() != TOKEN_SPECIAL)
			{
				throw "expected closing quote";
			}
			else
			{
				this->NextToken = new CToken(cBuffer, true);
			}
			break;
		// Unquoted string.
		case TOKEN_CHAR:
			cBuffer[uiIndex++] = Token->GetChar();

			while(this->ByteTokenizer->Peek()->GetToken() == TOKEN_CHAR)
			{
				Token = this->ByteTokenizer->Next();

				cBuffer[uiIndex++] = Token->GetChar();
			}
			cBuffer[uiIndex++] = '\0';

			check(uiIndex <= 4096);

			this->NextToken = new CToken(cBuffer, false);
			break;
		// Let these byte tokens "pass through".
		case TOKEN_EOF:
		case TOKEN_NEWLINE:
		case TOKEN_OPEN_BRACE:
		case TOKEN_CLOSE_BRACE:
			this->NextToken = new CToken(*Token);
			break;
		// The parser doesn't care about anything else.
		default:
			throw "unexpected token";
			break;
		}
	}

	CToken *Next()
	{
		delete this->CurrentToken;
		this->CurrentToken = this->NextToken;
		this->NextToken = 0;

		this->GetNextToken();

		return this->CurrentToken;
	}

	CToken *Peek()
	{
		return this->NextToken;
	}

	unsigned int GetLine()
	{
		return this->ByteTokenizer->GetLine();
	}
};

// Uses multi byte tokenizer to process the file.
class CParser
{
private:
	CTokenizer *Tokenizer;

public:
	CParser(CTokenizer *Tokenizer) : Tokenizer(Tokenizer)
	{

	}

public:
	CVMTGroupNode *Parse()
	{
		CToken *Token;
		CVMTGroupNode *Group = 0;

		// Consume all newlines.
		Token = this->Tokenizer->Next();
		while(Token->GetToken() == TOKEN_NEWLINE)
		{
			Token = this->Tokenizer->Next();
		}

		if(Token->GetToken() == TOKEN_STRING || Token->GetToken() == TOKEN_QUOTED_STRING)
		{
			Group = new CVMTGroupNode(Token->GetString());
		}
		else
		{
			throw "expected shader name";
		}

		// We *may* have a group, parse it.
		this->Parse(Group);

		if(uiVMTParseMode == PARSE_MODE_LOOSE)
		{
			while(true)
			{
				// Consume all newlines.
				while(this->Tokenizer->Peek()->GetToken() == TOKEN_NEWLINE)
				{
					Token = this->Tokenizer->Next();
				}

				CToken *Peek = this->Tokenizer->Peek();

				if(Peek->GetToken() == TOKEN_EOF)
				{
					break;
				}
				else if(Peek->GetToken() == TOKEN_OPEN_BRACE)
				{
					CVMTGroupNode *NextGroup = 0;
					try
					{
						NextGroup = new CVMTGroupNode("");
						this->Parse(NextGroup);
					}
					catch(char *cErrorMessage)
					{
						delete NextGroup;
						throw cErrorMessage;
					}
					for(unsigned int i = 0; i < NextGroup->GetNodeCount(); i++)
					{
						Group->AddNode(NextGroup->GetNode(i)->Clone());
					}
					delete NextGroup;
				}
				else
				{
					throw "expected end of file";
				}
			}
		}
		else
		{
			// Consume all newlines.
			Token = this->Tokenizer->Next();
			while(Token->GetToken() == TOKEN_NEWLINE)
			{
				Token = this->Tokenizer->Next();
			}

			if(Token->GetToken() != TOKEN_EOF)
			{
				throw "expected end of file";
			}
		}

		return Group;
	}

private:
	// Prase a group starting at the first brace and ending at the last.
	void Parse(CVMTGroupNode *Group)
	{
		CToken *Token;

		// Consume all newlines.
		Token = this->Tokenizer->Next();
		while(Token->GetToken() == TOKEN_NEWLINE)
		{
			Token = this->Tokenizer->Next();
		}

		// The first token better be an open brace.
		if(Token->GetToken() != TOKEN_OPEN_BRACE)
		{
			throw "expected open brace";
		}

		// Parse remaining tokens.
		while(true)
		{
			// Consume all newlines.
			Token = this->Tokenizer->Next();
			while(Token->GetToken() == TOKEN_NEWLINE)
			{
				Token = this->Tokenizer->Next();
			}

			// If we have an end brace, we found the end of the group.
			if(Token->GetToken() == TOKEN_CLOSE_BRACE || (uiVMTParseMode == PARSE_MODE_LOOSE && Token->GetToken() == TOKEN_EOF))
			{
				return;
			}

			// If we have a string we could have a pair or nested group.
			if(Token->GetToken() == TOKEN_STRING || Token->GetToken() == TOKEN_QUOTED_STRING)
			{
				CToken *Peek = this->Tokenizer->Peek();
				if(Peek->GetToken() == TOKEN_STRING || Peek->GetToken() == TOKEN_QUOTED_STRING)
				{
					// We have a pair.

					if (Peek->GetToken() == TOKEN_QUOTED_STRING)
					{
						Group->AddStringNode(Token->GetString(), Peek->GetString());

						Token = this->Tokenizer->Next();
					}
					else
					{
						char *cName = new char[strlen(Token->GetString()) + 1];
						strcpy_s(cName, strlen(Token->GetString()) + 1, Token->GetString());

						// Some materials contain properties such as '"$envmaptint" .1 .1 .1', we need to read
						// the .1's as strings and concat them (way to be consistent Valve).
						char cBuffer[4096] = "";
						while(this->Tokenizer->Peek()->GetToken() == TOKEN_STRING)
						{
							Token = this->Tokenizer->Next();

							if(*cBuffer)
							{
								strcat_s(cBuffer, sizeof(cBuffer), " ");
							}
							strcat_s(cBuffer, sizeof(cBuffer), Token->GetString());
						}

						int iTest;
						float sTest;
						char cTest[4096];

						if(sscanf_s(cBuffer, "%d%s", &iTest, cTest, (unsigned int)sizeof(cTest)) == 1)
						{
							// We can interpet the string as an integer, assume it is one.
							Group->AddIntegerNode(cName, iTest);
						}
						else if(sscanf_s(cBuffer, "%f%s", &sTest, cTest, (unsigned int)sizeof(cTest)) == 1)
						{
							// We can interpet the string as an single, assume it is one.
							Group->AddSingleNode(cName, sTest);
						}
						else
						{
							// The string must be a string...
							Group->AddStringNode(cName, cBuffer);
						}

						delete []cName;
					}

					bool bNeedNewline = Token->GetToken() != TOKEN_QUOTED_STRING;
					if(bNeedNewline)
					{
						Token = this->Tokenizer->Next();
						if(Token->GetToken() != TOKEN_NEWLINE)
						{
							throw "expected newline";
						}
					}
				}
				else if(Peek->GetToken() == TOKEN_NEWLINE || Peek->GetToken() == TOKEN_OPEN_BRACE)
				{
					// We have a nested group, parse it.
					this->Parse(Group->AddGroupNode(Token->GetString()));
				}
				else
				{
					throw "expected open brace or attribute value";
				}
			}
			else
			{
				throw "expected close brace or group name or attribute name";
			}
		}
	}
};

//
// Load()
// Parses a .vmt file.  Note, the parser is very loose.  .vmt files vary
// so much in the official resources that it is hard to know what is legal.
//
bool CVMTFile::Load(IO::Readers::IReader *Reader)
{
	delete this->Root;
	this->Root = 0;

	if(!Reader->Open())
		return false;

	CByteTokenizer ByteTokenizer = CByteTokenizer(Reader);
	CTokenizer Tokenizer = CTokenizer(&ByteTokenizer);
	CParser Parser = CParser(&Tokenizer);

	try
	{
		this->Root = Parser.Parse();
	}
	catch(char *cErrorMessage)
	{
		LastError.SetFormatted("Error parsing material on line %u (%s).", Tokenizer.GetLine(), cErrorMessage);
	}

	Reader->Close();

	return this->Root != 0;
}

/*bool CVMTFile::Load(IO::Readers::IReader *Reader)
{
	delete this->Root;
	this->Root = 0;

	if(!Reader->Open())
		return false;

	try
	{
		CVMTNode *Node = this->Load(Reader, false);

		// Make sure we loaded a group.
		if(Node->GetType() == NODE_TYPE_GROUP)
		{
			this->Root = static_cast<CVMTGroupNode *>(Node);
		}
		else
		{
			delete Node;
		}
	}
	catch(...)
	{
		LastError.Set("Error parsing material.");
	}

	Reader->Close();

	return this->Root != 0;
}*/

// This old load code wasn't "loose" enough and had problems
// with *malformed* .vmt files.  Too bad, it was compact.  This
// could be modified to read the stricter configuration files with
// ease.

//
// Load()
// Reads the next node.  Returns a group node, value node or
// null (if there is nothing to parse).  Throws an exception if
// there is an error parsing the file.
//
/*CVMTNode *CVMTFile::Load(IO::Readers::IReader *Reader, bool bInGroup)
{
	char cChar;

	unsigned int uiNameLength, uiValueLength;
	char cNameBuffer[1024], cValueBuffer[1024];

	while(true)
	{
		if(!Reader->Read(cChar))
		{
			if(bInGroup)	// We are expecting a '}'.
				throw 0;
			else
				return 0;	// We are expecting nothing, we are done.
		}

		if(isspace(cChar))
			continue;

		if(cChar == '\"')	// We have the start of something...
		{
			// Read the string.
			uiNameLength = 0;
			while(true)
			{
				if(!Reader->Read(cChar))
					throw 0;

				if(cChar == '\"')	// We found the end of the string.
					break;

				cNameBuffer[uiNameLength++] = cChar;
			}
			cNameBuffer[uiNameLength++] = '\0';

			// Find out if we have a group or value.
			while(true)
			{
				if(!Reader->Read(cChar))
					throw 0;

				if(isspace(cChar))
					continue;

				if(cChar == '{')	// We have a group.
				{
					CVMTGroupNode *Group = new CVMTGroupNode(cNameBuffer);

					try	// Cleanup resources reverse-recursively on error.
					{
						while(true)
						{
							CVMTNode *Node = this->Load(Reader, true);
							
							if(Node == 0)
								break;

							// We can do this because we are friends.
							Group->AddNode(Node);
						}
					}
					catch(...)
					{
						delete Group;
						throw 0;
					}

					return Group;
				}
				else if(cChar == '\"')	// We have a string value.
				{
					// Read the value (string).
					uiValueLength = 0;
					while(true)
					{
						if(!Reader->Read(cChar))
							throw 0;

						if(cChar == '\"')	// We found the end of the string.
							break;

						cValueBuffer[uiValueLength++] = cChar;
					}
					cValueBuffer[uiValueLength++] = '\0';

					return new CVMTStringNode(cNameBuffer, cValueBuffer);
				}
				else if((cChar >= '0' && cChar <= '9') || cChar == '.' || cChar == '+' || cChar == '-')	// We have a numeric value.
				{
					bool bInteger = cChar != '.';

					// Read the value (numeric).
					uiValueLength = 0;
					cValueBuffer[uiValueLength++] = cChar;
					while(true)
					{
						if(!Reader->Read(cChar))
							throw 0;

						if(isspace(cChar))	// We found the end of the number.
							break;

						if(cChar == '.')
						{
							if(bInteger)
								bInteger = false;
							else
								throw 0;
						}

						if((cChar < '0' || cChar > '9') && cChar != '.')
							throw 0;

						cValueBuffer[uiValueLength++] = cChar;
					}
					cValueBuffer[uiValueLength++] = '\0';

					if(bInteger)
					{
						return new CVMTIntegerNode(cNameBuffer, cValueBuffer);
					}
					else
					{
						return new CVMTSingleNode(cNameBuffer, cValueBuffer);
					}
				}
				else if(cChar == '/')	// We have a comment, scan past it.
				{
					if(!Reader->Read(cChar))
						throw 0;

					if(cChar != '/')
						throw 0;

					while(true)
					{
						if(!Reader->Read(cChar))
							throw 0;

						if(cChar == '\n')	// We found the end of the comment.
							break;
					}
				}
				else	// We have a problem. ;)
				{
					throw 0;
				}
			}
		}
		else if(cChar == '/')	// We have a comment, scan past it.
		{
			if(!Reader->Read(cChar))
				throw 0;

			if(cChar != '/')
				throw 0;

			while(true)
			{
				if(!Reader->Read(cChar))
					throw 0;

				if(cChar == '\n')	// We found the end of the comment.
					break;
			}
		}
		else if(cChar == '}' && bInGroup)	// We could have the end of a group (if we are looking for it).
		{
			return 0;
		}
		else	// We have a problem. ;)
		{
			throw 0;
		}
	}
}*/

bool CVMTFile::Save(IO::Writers::IWriter *Writer) const
{
	if(this->Root == 0)
	{
		LastError.Set("No material loaded.");
		return false;
	}

	if(!Writer->Open())
		return false;

	this->Save(Writer, this->Root);

	Writer->Close();

	return true;
}

//
// Indent()
// Indents a line uiLevel tab sapces.
//
void CVMTFile::Indent(IO::Writers::IWriter *Writer, unsigned int uiLevel) const
{
	for(unsigned int i = 0; i < uiLevel; i++)
	{
		Writer->Write('\t');
	}
}

//
// Save()
// Saves a node to a file.
//
void CVMTFile::Save(IO::Writers::IWriter *Writer, CVMTNode *Node, unsigned int uiLevel) const
{
	char cBuffer[2048];

	if(Node->GetType() == NODE_TYPE_GROUP)
	{
		CVMTGroupNode *Group = static_cast<CVMTGroupNode *>(Node);

		this->Indent(Writer, uiLevel);
		sprintf_s(cBuffer, sizeof(cBuffer), "\"%s\"\r\n", Group->GetName());
		Writer->Write(cBuffer, (unsigned int)strlen(cBuffer));

		this->Indent(Writer, uiLevel);
		sprintf_s(cBuffer, sizeof(cBuffer), "{\r\n");
		Writer->Write(cBuffer, (unsigned int)strlen(cBuffer));

		for(unsigned int i = 0; i < Group->GetNodeCount(); i++)
		{
			this->Save(Writer, Group->GetNode(i), uiLevel + 1);
		}

		this->Indent(Writer, uiLevel);
		sprintf_s(cBuffer, sizeof(cBuffer), "}\r\n");
		Writer->Write(cBuffer, (unsigned int)strlen(cBuffer));
	}
	else if(Node->GetType() == NODE_TYPE_STRING)
	{
		CVMTStringNode *String = static_cast<CVMTStringNode *>(Node);

		this->Indent(Writer, uiLevel);
		sprintf_s(cBuffer, sizeof(cBuffer), "\"%s\" \"%s\"\r\n", String->GetName(), String->GetValue());
		Writer->Write(cBuffer, (unsigned int)strlen(cBuffer));
	}
	else if(Node->GetType() == NODE_TYPE_INTEGER)
	{
		CVMTIntegerNode *Integer = static_cast<CVMTIntegerNode *>(Node);

		this->Indent(Writer, uiLevel);
		sprintf_s(cBuffer, sizeof(cBuffer), "\"%s\" %d\r\n", Integer->GetName(), Integer->GetValue());
		Writer->Write(cBuffer, (unsigned int)strlen(cBuffer));
	}
	else if(Node->GetType() == NODE_TYPE_SINGLE)
	{
		CVMTSingleNode *Single = static_cast<CVMTSingleNode *>(Node);

		this->Indent(Writer, uiLevel);
		sprintf_s(cBuffer, sizeof(cBuffer), "\"%s\" %f\r\n", Single->GetName(), Single->GetValue());
		Writer->Write(cBuffer, (unsigned int)strlen(cBuffer));
	}
}

CVMTGroupNode *CVMTFile::GetRoot() const
{
	return this->Root;
}