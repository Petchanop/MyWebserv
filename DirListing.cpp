#include "Response.hpp"

void	Response::printFileSize(std::ostringstream &oss, const std::string &name) {
	struct stat statbuf;

	if (stat(name.c_str(), &statbuf) == 0) {
		if (statbuf.st_size >= 1000) {
			oss << statbuf.st_size / 1000 << " kB";
		} else {
			oss << statbuf.st_size << " B";
		}
	}
}

void	Response::printDateModified(std::ostringstream &oss, const std::string &name) {
	struct stat statbuf;

	if (stat(name.c_str(), &statbuf) == 0) {
		char mbstr[100];
		if (std::strftime(mbstr, sizeof(mbstr),
						  "%D %r", std::localtime(&statbuf.st_mtime)))
			oss << mbstr;
	}
}

void	Response::printStyle(std::ostringstream &oss) {
	oss << "<style>"
		<< "td {padding: 0 2em}"
		<< "</style>";
}

void	Response::printTable(std::ostringstream &oss, std::stringstream &ss) {
	std::string	path(_fullPath), dname;

	oss << "<table>"
	   << "<thead><tr><th>Name</th><th>Size</th><th>Date Modified</th></tr></thead>"
	   << "<tbody>";

	while (std::getline(ss, dname)) {
		if (dname == "." || dname == "..")
			continue ;
		path.replace(path.rfind('/') + 1, std::string::npos, dname);
		oss << "<tr>";
		oss << "<td><a href=\"" << dname << "\" download>" << dname << "</a></td>";
		oss << "<td class=\"\">";
		printFileSize(oss, path);
		oss << "</td>";
		oss << "<td>";
		printDateModified(oss, path);
		oss << "</td>";
		oss << "</tr>";
	}
	
	oss << "</tobody>"
	   << "</table>";
}

void	Response::directoryListing() {
	std::ostringstream	oss;
	std::stringstream	dirList;
	DIR				*dir;
	struct dirent	*ent;

	oss << "<!DOCYTPE html><html>"
		<< "<head>";
	printStyle(oss);
	oss << "</head>"
		<< "<body>"
		<< "<h1>Index of " << _request.getUri() << "</h1>"
		<< "<p><a href=\"..\">[parent directory]</a></p>";
	if ((dir = opendir(_fullPath.c_str()))) {
		while ((ent = readdir(dir)))
			dirList << ent->d_name << "\n";
		closedir(dir);
	}
	printTable(oss, dirList);
	oss << "</body>"
		<< "</html>";

	_msgBody = oss.str();
	setContentLength();
	_headers["Content-Type"] = "text/html";
}
