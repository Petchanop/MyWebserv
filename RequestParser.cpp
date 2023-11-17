#include "RequestParser.hpp"

RequestParser::RequestParser(void): _msgBody(NULL), _msgLen(0), _readLen(0)
{
}

RequestParser::RequestParser(const RequestParser &src): _msgBody(NULL), _msgLen(0), _readLen(0)
{
	*this = src;
}

RequestParser::~RequestParser(void)
{
	delete[] _msgBody;
}

RequestParser &RequestParser::operator= (const RequestParser &src)
{
	if (this != &src)
	{
		_reqLine = src._reqLine;
		_headers = src._headers;
		_msgBody = strdup(src._msgBody);
		_msgLen = src._msgLen;
		_readLen = src._readLen;
	}
	return *this;
}

size_t	RequestParser::readToBuf(int sockfd, char *&buf)
{
	size_t	bufsize = _readLen;
	char	recvbuf[MAXLINE] = {0};
	int		tmpReadLen = 0;

	do
	{
		tmpReadLen = recv(sockfd, recvbuf, MAXLINE, 0);
		if (tmpReadLen == 0)
		{
			return bufsize > _readLen ? bufsize : 0;
		}
		else if (tmpReadLen < 0)
		{
			perror("recv");
			return 0;
		}
		buf = (char *)realloc(buf, bufsize + tmpReadLen + 1);
		memmove(buf + bufsize, recvbuf, tmpReadLen);
		buf[bufsize + tmpReadLen] = 0;
		bufsize += tmpReadLen + 1;
	} while (tmpReadLen >= MAXLINE);
	return bufsize;
}

void	RequestParser::readRequest(int sockfd)
{
	// char	buf[MAXLINE] = {0};
	char *buf = NULL;
	std::string temp = "";
	size_t	tmpReadLen = 0;

	do
	{
		tmpReadLen = readToBuf(sockfd, buf);
		if (tmpReadLen == 0)
		{
			_readLen = 0;
			free(buf);
			return ;
		}
		_readLen += tmpReadLen;
	} while (strstr(buf, "\r\n\r\n") == NULL);
	std::istringstream	iss(buf);
	std::string			line;
	while (std::getline(iss, line))
		if (!line.empty())
			break;
	parseRequestLine(line);
	parseHeaders(iss);
	// std::cout << "requestline ="
	// for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
			// it != _headers.end(); it++)
	// {
		// std::cout << "header : " << it->first << " : " << it->second << std::endl;
	// }
	try
	{
		parseMessageBody(iss.tellg(), sockfd, buf);
		if (_msgLen > 0)
		{
			FILE	*fp = fopen("./html/download/myfile.bin", "wb");
			if (fp == NULL)
			{
				// Error handling.
			}
			fwrite(_msgBody, sizeof(_msgBody[0]), _msgLen, fp);
			fclose(fp);
		}
	} catch (int)
	{
		_readLen = 0;
	}

	free(buf);
}

void	RequestParser::parseRequestLine(const std::string &line)
{
	std::istringstream		iss(line);
	std::string::size_type	queryPos;

	if (iss >> _reqLine.method >> _reqLine.uri >> _reqLine.version >> std::ws)
	{
		if ((queryPos = getUri().find('?')) != std::string::npos)
		{
			_reqLine.query = _reqLine.uri.substr(queryPos + 1);
			_reqLine.uri.erase(queryPos);
		}
		if (_reqLine.uri[0] != '/')
			_reqLine.uri.insert(_reqLine.uri.begin(), '/');
	}
	// throw
}

void	RequestParser::parseHeaders(std::istringstream &src)
{
	std::stringstream	ss;
	std::string			line, key, val;

	while (std::getline(src, line))
	{
		if (line.empty() || line == "\r")
			break;

		ss.clear();
		ss.str(line);
		ss >> key >> std::ws;

		if (key[key.length() - 1] == ':')
		{
			key.erase(key.length() - 1);
			val = ss.str().substr(ss.tellg());
			if (val.find("\r") != std::string::npos)
				val.erase(val.find("\r"));
			_headers[key] = val;
		}
	}
}

void	RequestParser::parseMessageBody(int i, int sockfd, char *&buf)
{
	size_t	tmpReadLen = 0;
	std::cout << "msgparse : " << i << std::endl;
	if (getMethod() == "POST")
	{
		_msgLen = atoi(_headers.at("Content-Length").c_str());
		while (_readLen - i < _msgLen)
		{
			tmpReadLen = readToBuf(sockfd, buf);
			if (tmpReadLen == 0)
			{
				throw 0;
			}
			_readLen = tmpReadLen;
		}
	}
	_msgBody = new char[_msgLen + 1];
	memmove(_msgBody, buf, _msgLen);
	_msgBody[_msgLen] = 0;
}

char	**RequestParser::toEnv(const t_locationData &servLoc, char **&env)
{
	std::map<std::string, std::string>	envMap;
	std::string				host, key, line;
	std::string::size_type	pos;

	host = _headers.at("Host");

	envMap["SERVER_PROTOCOL"] = getVersion();
	if ((pos = host.find(':')) != std::string::npos)
		envMap["SERVER_PORT"] = host.substr(pos + 1);
	envMap["REQUEST_METHOD"] = getMethod();

	pos = getUri().find('.');
	if ((pos = getUri().find('/', pos)) != std::string::npos)
	{
		envMap["PATH_INFO"] = getUri().substr(pos);
		envMap["PATH_TRANSLATED"] = servLoc.root + getUri().substr(pos);
	}
	envMap["SERVER_PROTOCOL"] = getVersion();
	envMap["SCRIPT_NAME"] = getUri().substr(0, pos);
	envMap["SCRIPT_FILENAME"] = servLoc.root + getUri().substr(0, pos);
	if (!getQuery().empty())
		envMap["QUERY_STRING"] = getQuery();

	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
			it != _headers.end(); it++)
	{
		key = it->first;
		while ((pos = key.find('-')) != std::string::npos)
			key.replace(pos, 1, "_");
		std::transform(key.begin(), key.end(), key.begin(), ::toupper);
		if (key == "CONTENT_LENGTH" || key == "CONTENT_TYPE")
			envMap[key] = it->second;
		else
			envMap["HTTP_" + key] = it->second;
	}

	env = new char*[envMap.size() + 2];
	int	i = 0;
	for (std::map<std::string, std::string>::const_iterator it = envMap.begin();
			it != envMap.end(); it++)
	{
		line = it->first + "=";
		line += it->second;
		env[i] = new char[line.length() + 1];
		strcpy(env[i++], line.c_str());
	}
	line = "REDIRECT_STATUS=200";
	env[i] = new char[line.length() + 1];
	strcpy(env[i++], line.c_str());
	env[i] = NULL;
	return env;
}

const std::string	&RequestParser::getMethod() const
{
	return _reqLine.method;
}

const std::string	&RequestParser::getUri() const
{
	return _reqLine.uri;
}

const std::string	&RequestParser::getQuery() const
{
	return _reqLine.query;
}

const std::string	&RequestParser::getVersion() const
{
	return _reqLine.version;
}

const std::map<std::string, std::string>	&RequestParser::getHeaders() const
{
	return _headers;
}

const char	*RequestParser::getMessageBody() const
{
	return _msgBody;
}

const size_t	&RequestParser::getMessageBodyLen() const
{
	return _msgLen;
}

const size_t	&RequestParser::getReadLen() const
{
	return _readLen;
}

RequestParser::t_reqLine &RequestParser::t_reqLine::operator=(const t_reqLine &src)
{
	if (this == &src)
	{
		method = src.method;
		uri = src.uri;
		query = src.query;
		version = src.version;
	}
	return *this;
}
