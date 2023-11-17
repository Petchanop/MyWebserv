#include "Response.hpp"

Response::Response(void): _cgi(this)
{
	initStatusMapping();
}

Response::Response(const Response &src)
{
	*this = src;
}

Response::Response(const t_serverData &serv, int sockfd): _cgi(this)
{
	_request.readRequest(sockfd);
	if (_request.getReadLen() == 0)
		return ;

	initStatusMapping();
	try
	{
		setRequestLocation(serv);
		methodHandler();
	}
	catch (const int &e)
	{
		_code = e;
	}
	catch (...)
	{
		return;
	}
	setResponse();
}

Response::~Response(void)
{
}

Response &Response::operator=(const Response &src)
{
	if (this != &src)
	{
		_cgi = src._cgi;
		_request = src._request;
		_fullPath = src._fullPath;
		_response = src._response;
		_code = src._code;
		_headers = src._headers;
		_msgBody = src._msgBody;
		_statMaping = src._statMaping;
	}
	return *this;
}

void Response::initStatusMapping()
{
	_statMaping[0] = "";

	_statMaping[200] = "OK";
	_statMaping[201] = "Created";
	_statMaping[202] = "Accepted";
	_statMaping[203] = "Non-Authoritative Information";
	_statMaping[204] = "No Content";
	_statMaping[205] = "Reset Content";
	_statMaping[206] = "Partial Content";

	_statMaping[300] = "Multiple Choices";
	_statMaping[301] = "Moved Permanently";
	_statMaping[302] = "Found";
	_statMaping[303] = "See Other";
	_statMaping[304] = "Not Modified";
	_statMaping[305] = "Use Proxy";
	_statMaping[307] = "Temporary Redirect";

	_statMaping[400] = "Bad Request";
	_statMaping[401] = "Unauthorized";
	_statMaping[403] = "Forbidden";
	_statMaping[404] = "Not Found";
	_statMaping[405] = "Method Not Allowed";
	_statMaping[406] = "Not Acceptable";
	_statMaping[413] = "Content Too Large";

	_statMaping[500] = "Internal Server Error";
	_statMaping[501] = "Not Implemented";
	_statMaping[502] = "Bad Gateway";
	_statMaping[503] = "Service Unavailable";
	_statMaping[504] = "Gateway Time-out";
	_statMaping[505] = "HTTP Version not supported";
}

void Response::setRequestLocation(const t_serverData &serv)
{
	t_listenData lsn;
	std::string::size_type pos;

	lsn.addr = _request.getHeaders().at("Host");
	if ((pos = lsn.addr.find(':')) != std::string::npos)
	{
		lsn.port = atoi(lsn.addr.substr(pos + 1).c_str());
		lsn.addr.erase(pos);
	}
	else
	{
		// lsn.port = 80;
	}

	if (isIPv4(lsn.addr) || std::find(serv.name.begin(), serv.name.end(), lsn.addr) != serv.name.end())
	{
		for (std::vector<t_locationData>::const_iterator it = serv.location.begin();
				it != serv.location.end(); it++)
		{
			for (std::vector<t_listenData>::const_iterator it2 = it->listen.begin();
					it2 != it->listen.end(); it2++)
			{
				// std::cout << it->location
				if ((!isIPv4(lsn.addr) || it2->addr == lsn.addr) && it2->port == lsn.port)
				{
					if (_request.getUri().find(it->uri) == 0)
					{
						_reqLoc = *it;
						return;
					}
				}
			}
		}
	}
	throw 400;
}

void Response::setResponse()
{
	std::ostringstream oss;

	// std::cout << "CODE " << _code << "\n";
	if (!(_code == 200 || _code==302))
	{
		setErrorPath();
		setMessageBody();
		setContentType();
		setContentLength();
	}
	oss << getStatusLine() << getHeadersText() << "\r\n"
		<< _msgBody;
	_response = oss.str();
}

void	Response::setCode(const int c) {
	_code = c;
}

void	Response::setHeader(const std::string &k, const std::string &v) {
	_headers[k] = v;
}

void Response::setMessageBody()
{
	std::ostringstream oss;
	std::ifstream ifs(_fullPath.c_str());

	_msgBody.assign(std::istreambuf_iterator<char>(ifs),
					std::istreambuf_iterator<char>());
	ifs.close();
}

void	Response::setMessageBody(const std::string &s) {
	_msgBody = s;
}

bool Response::setFullPath()
{
	std::ostringstream oss;
	struct stat statBuf;

	// if request index
	if (_request.getUri() == "/")
	{
		for (std::vector<std::string>::const_iterator it = _reqLoc.index.begin();
			 it != _reqLoc.index.end(); it++)
		{
			oss << _reqLoc.root << '/' << *it;
			if (stat(oss.str().c_str(), &statBuf) == 0)
			{
				_fullPath = oss.str();
				return S_ISDIR(statBuf.st_mode);
			}
			oss.clear();
			oss.str("");
		}
	}
	else
	{
		oss << _reqLoc.root;
		// if location doesn't overridden root
		if (!_reqLoc.isRootOvr) 
			oss << _request.getUri();
		else
			oss << _request.getUri().substr(_request.getUri().find(_reqLoc.uri) + _reqLoc.uri.length());
		// std::cout << "full: " << oss.str() << '\n';
		if (stat(oss.str().c_str(), &statBuf) == 0)
		{
			_fullPath = oss.str();
			return S_ISDIR(statBuf.st_mode);
		}
	}
	if (errno == EACCES)
		throw 403;
	else if (errno == ENOENT)
		throw 404;
	return 0;
}

void Response::setErrorPath()
{
	std::ostringstream oss;
	struct stat statBuf;

	for (std::vector<t_errorPageData>::const_iterator it = _reqLoc.errPage.begin();
		 it != _reqLoc.errPage.end(); it++)
	{
		if (std::find(it->code.begin(), it->code.end(), _code) != it->code.end())
		{
			oss << _reqLoc.root << it->uri;
			if (stat(oss.str().c_str(), &statBuf) == 0)
			{
				_fullPath = oss.str();
				return;
			}
		}
	}
	oss << std::getenv("PWD") << "/webserv_default_error/error.html";
	_fullPath = oss.str();
}

void Response::setDate()
{
	std::time_t t = std::time(0);
	char mbstr[100];

	if (std::strftime(mbstr, sizeof(mbstr),
					  "%a, %d %b %Y %T GMT", std::gmtime(&t)))
	{
		_headers["Date"] = std::string(mbstr);
	}
}

void Response::setLocation()
{
	std::ostringstream oss;

	oss << "http://"
		<< _request.getHeaders().at("Host")
		<< (*_request.getUri().begin() == '/' ? "" : "/")
		<< _request.getUri()
		<< '/';

	// std::cout << "location: " << oss.str() << "\n";
	_headers["Location"] = oss.str();
}

void Response::setContentLength()
{
	std::ostringstream oss;

	oss << _msgBody.length();
	_headers["Content-Length"] = oss.str();
}

// https://www.iana.org/assignments/media-types/media-types.xhtml
void Response::setContentType()
{
	std::string::size_type pos = _fullPath.rfind('.') + 1;
	std::string type = "text/plain";

	if (pos != std::string::npos)
	{
		std::string ext = _fullPath.substr(pos);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		if (ext == "jpeg" || ext == "jpg")
			type = "image/jpeg";
		else if (ext == "png")
			type = "image/png";
		else if (ext == "css")
			type = "text/css";
		else if (ext == "html")
			type = "text/html";
	}

	// if (_request.getHeaders().at("Accept").find("*/*") == std::string::npos) {
	// 	if (_request.getHeaders().at("Accept").find(type) == std::string::npos)
	// 		throw 406;
	// }

	_headers["Content-Type"] = type;
}

void Response::methodHandler()
{
	std::map<std::string, void (Response::*)()> method;

	method["GET"] = &Response::methodGet;
	method["POST"] = &Response::methodPost;
	method["DELETE"] = &Response::methodDelete;

	if (method.find(_request.getMethod()) != method.end())
	{
		std::cout << _request.getMethod() << "\n";
		std::map<std::string, std::string> headers = getHeaders();
		std::cout << "check header\n";
		std::cout << getHeadersText() << "\n";
		if (std::find(_reqLoc.limExcept.begin(), _reqLoc.limExcept.end(),
					  _request.getMethod()) != _reqLoc.limExcept.end())
			(this->*method[_request.getMethod()])();
		// else if (_request.getMethod() == "POST")
		// 	(this->*method[_request.getMethod()])();
		else
			throw 405;
	}
	else
	{
		throw 501;
	}
}

void Response::methodGet()
{
	setDate();

	// if full path is directory
	if (setFullPath())
	{
		if (_reqLoc.autoIdx == "on")
		{
			if (*(_fullPath.end() - 1) != '/')
			{
				setLocation();
				throw 301;
			}
			directoryListing();
		}
		else
			throw 403;
	}
	else if (_fullPath.find(".php") == std::string::npos)
	{
		setMessageBody();
		setContentLength();
		setContentType();
		if (_fullPath.find(_request.getUri()) != std::string::npos)
		{
			int	pipeinfd[2];
			int pipeoutfd[2];

			if (pipe(pipeinfd) == -1 || pipe(pipeoutfd) == -1) {
				perror("error pipe");
				exit(EXIT_FAILURE);
			}
			std::string filename = "filename=";
			std::string fullpath = "fullpath=" ;
			std::string filedes = "fd=";
			std::ostringstream num;
			num << pipeoutfd[1];
			filedes.append(num.str());
			fullpath.append(&_fullPath.c_str()[1], _fullPath.length());
			size_t pos = _request.getUri().find("download/");
			if (pos != std::string::npos)
				filename.append(_request.getUri().substr(pos + 9, _request.getUri().length() - pos - 9));
			char *fname = const_cast<char *>(filename.c_str());
			char cgi[27] = "cgi-bin/download-file.py";
			char *save_path = const_cast<char*>(fullpath.c_str());
			char *fd = const_cast<char *>(filedes.c_str());
			char *arg[2] = {cgi, NULL};
			char *env[4] = {save_path, fname, fd, NULL};
			int pid = fork();
			if (pid < 0)
				perror("fork failed : ");
			if (pid == 0) {
				close(pipeinfd[1]); //close read
				close(pipeoutfd[0]);
				dup2(pipeinfd[0], STDIN_FILENO);
				dup2(pipeoutfd[1], STDOUT_FILENO);
				close(pipeinfd[0]);
				close(pipeoutfd[0]);
				if (execve(arg[0], &arg[0], env) == -1){
					perror("");
					std::cout << "execve failed\n";
				}
				exit(0);
			}
			close(pipeoutfd[1]); //close write
			close(pipeinfd[0]);
			close(pipeinfd[1]);
			char	buf[MAXLINE];
			int status;
			std::string temp  = "";
			int		bytes_read = 1;
			while (bytes_read > 0) {
				bytes_read = read(pipeoutfd[0], buf, MAXLINE);
				if (bytes_read < 0){
					perror("read failed : ");
					break;
				}
				temp.append(buf, bytes_read);
			}
			close(pipeoutfd[0]);
			waitpid(pid, &status, 0);
			_response = temp;
		}
	}
	else
	{
		_cgi.executeCgi(_reqLoc.cgiPass);
		return;
	}

	_code = 200;
}

void Response::methodPost()
{
	setDate();
	std::cout << "methodPost\n";
	// if (_reqLoc.cliMax != 0)
	// 	if (_request.getMessageBodyLen() > _reqLoc.cliMax)
	// 		throw 413;
	// _cgi.executeCgi(_reqLoc.cgiPass);
	std::map<std::string, std::string> headers = getHeaders();
	std::cout << headers["Content-Type"] << std::endl;
	int fd = open("./html/download/myfile.bin", O_RDONLY);
	// fcntl(fd, F_SETFL, O_NONBLOCK);
	std::ostringstream num, writefd;
	num << fd;
	std::cout << "sockd fd : " << fd << std::endl;
	char *sock = new char[num.str().length() + 1];
	strcpy(sock,const_cast<char *>(num.str().c_str()));
	char cmd[18] = "./cgi-bin/testcgi";
	char *arg[4] = {cmd, sock, sock, NULL};
	int pid = fork();
	if (pid == 0) {
		if (execve(arg[0], &arg[0], NULL) == -1){
			perror("");
			std::cout << "execve failed\n";
		}
	}
	waitpid(pid, NULL, 0);
	delete[] sock;
	close(fd);
	setMessageBody();
	setContentLength();
	setContentType();
	_code = 200;
}

void Response::methodDelete()
{
	int	pipeinfd[2];
	int pipeoutfd[2];

	std::cout << "methodDelete\n";
	std::cout << "uri : " << _request.getUri() << std::endl;
	if (pipe(pipeinfd) == -1 || pipe(pipeoutfd) == -1) {
		perror("error pipe");
		exit(EXIT_FAILURE);
	}
	std::string filename = "filename=";
	std::string fullpath = "fullpath=" ;
	std::string filedes = "fd=";
	std::ostringstream num;
	num << pipeoutfd[1];
	filedes.append(num.str());
	fullpath.append(_request.getUri().c_str(), _request.getUri().length());
	size_t pos = _request.getUri().find("download/");
	if (pos != std::string::npos)
		filename.append(_request.getUri().substr(pos + 9, _request.getUri().length() - pos - 9));
	std::cout << "check filename : " << filename << std::endl;
	std::cout << "check fullpath : " << fullpath << std::endl;
	char *fname = const_cast<char *>(filename.c_str());
	char cgi[27] = "cgi-bin/delete-file.perl";
	char *save_path = const_cast<char*>(fullpath.c_str());
	char *fd = const_cast<char *>(filedes.c_str());
	char *arg[2] = {cgi, NULL};
	char *env[4] = {save_path, fname, fd, NULL};
	int pid = fork();
	if (pid < 0)
		perror("fork failed : ");
	if (pid == 0) {
		close(pipeinfd[1]); //close read
		close(pipeoutfd[0]);
		dup2(pipeinfd[0], STDIN_FILENO);
		dup2(pipeoutfd[1], STDOUT_FILENO);
		close(pipeinfd[0]);
		close(pipeoutfd[0]);
		if (execve(arg[0], &arg[0], env) == -1){
			perror("");
			std::cout << "execve failed\n";
		}
		exit(0);
	}
	close(pipeinfd[0]);
	close(pipeinfd[1]);
	char	buf[MAXLINE];
	int status;
	std::string temp  = "";
	int		bytes_read = 1;
	while (bytes_read > 0) {
		bytes_read = read(pipeoutfd[0], buf, MAXLINE);
		if (bytes_read < 0){
			perror("read failed : ");
			break;
		}
		temp.append(buf, bytes_read);
	}
	close(pipeoutfd[0]);
	waitpid(pid, &status, 0);
	std::cout << "delete : " << temp << std::endl;
	_response = temp;
}

char	**Response::toEnv(char **&env)
{
	return _request.toEnv(_reqLoc, env);
}

const std::string &Response::getResponse() const
{
	return _response;
}

const std::string Response::getStatusLine() const
{
	std::ostringstream oss;

	oss << _request.getVersion() << " "
		<< _code << " "
		<< _statMaping.at(_code) << "\r\n";
	return oss.str();
}

const std::string Response::getHeadersText() const
{
	std::ostringstream oss;

	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); it++)
		oss << it->first << ": " << it->second << "\r\n";
	return oss.str();
}

const std::string &Response::getMessageBody() const
{
	return _msgBody;
}

const std::map<std::string, std::string> &Response::getHeaders() const
{
	return	_headers;
}

const RequestParser	&Response::getRequest() const
{
	return _request;
}
