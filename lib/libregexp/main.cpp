# include "RegexParser.hpp"

void print_method(std::string str) { std::cout << "the method is: " + str << std::endl; }
void print_scheme(std::string str) { std::cout << "the scheme is: " + str << std::endl; }
void print_userinfo(std::string str) { std::cout << "the userinfo is: " + str << std::endl; }
void print_host(std::string str) { std::cout << "the host is: " + str << std::endl; }
void print_port(std::string str) { std::cout << "the port is: " + str << std::endl; }
void print_authorithy(std::string str) { std::cout << "the authorithy is: " + str << std::endl; }
void print_path(std::string str) { std::cout << "the path is: " + str << std::endl; }
void print_query(std::string str) { std::cout << "the query is: " + str << std::endl; }
void print_fragment(std::string str) { std::cout << "the fragment is: " + str << std::endl; }
void print_header_name(std::string str) { std::cout << "Header-Name: " + str; }
void print_header_value(std::string str) { std::cout << " Header-Value: " + str << std::endl; }

int main()
{
	std::vector<void(*)(std::string)> actions2;
	actions2.push_back(print_method);
	actions2.push_back(print_scheme);
	actions2.push_back(print_userinfo);
	actions2.push_back(print_host);
	actions2.push_back(print_port);
	actions2.push_back(print_authorithy);
	actions2.push_back(print_path);
	actions2.push_back(print_query);
	actions2.push_back(print_fragment);
	actions2.push_back(print_header_name);
	actions2.push_back(print_header_value);

	/* RFC 3986 regex */
	std::string pct_encoded = "%[abcdefABCDEF0123456789]{2,2}";
	std::string unreserved = "\\w\\d-._~";
	std::string sub_delims = "!$&',;=\\(\\)\\*\\+";
	std::string IPv4address = "([\\d]{1,3}.){3,3}[\\d]{1,3}";
	std::string reg_name = "(" + pct_encoded + "|[" + unreserved + sub_delims + "])+";
	std::string pchar = "(" + pct_encoded + "|[" + unreserved + sub_delims + ":@])+";
	std::string queryrgx = "(" + pct_encoded + "|[" + unreserved + sub_delims + ":@/\\?])+";
	/* RFC 3986 regex end */

	std::string method = "((DELETE|GET|POST) )~0";
	std::string scheme = "([\\w][" + unreserved + "]*:)~1?";
	std::string userinfo = "([" + unreserved + sub_delims + "]+@)~2?";
	std::string host = "(" + reg_name + ")~3?";
	std::string port = "(:[\\d]+)~4?";
	std::string authorithy = "(//" + userinfo + host + port + ")~5?";
	std::string path = "([^\\?# ]+)~6?";		// Es un poco ambiguo como se maneja el path...
	std::string query = "(\\?" + queryrgx + ")~7?";
	std::string fragment = "(#" + queryrgx + ")~8?";
	std::string protocol = " HTTP/1.1\r\n";
	std::string headerName = "([^:\r\n]+: )~9";	// Dos puntos y espacio obligatorios.

	std::string headerValue = "([^\r\n]+)~10\r\n";
	std::string headers = "(" + headerName + headerValue + ")*";
	std::string end = "\r\n";
	RegexParser regex(method + scheme + authorithy + path + query + fragment + protocol + headers + end, actions2);

	std::vector<std::string> URIs;
	URIs.push_back("GET https://john.doe@www.example.com:1234/forum/questions/?tag=networking&order=newest#top HTTP/1.1\r\n\r\n");
	URIs.push_back("GET https://www.example.com:1234/forum/questions/?tag=networking&order=newest#top HTTP/1.1\r\n\r\n");
	URIs.push_back("GET https://www.example.com/forum/questions/?tag=networking&order=newest#top HTTP/1.1\r\n\r\n");
	URIs.push_back("GET https://www.example.com/forum/questions/#top HTTP/1.1\r\n\r\n");
	URIs.push_back("GET https://www.example.com/forum/questions/ HTTP/1.1\r\n\r\n");
	URIs.push_back("GET /forum/questions/ HTTP/1.1\r\n\r\n");
	URIs.push_back("GET telnet://192.0.2.16:80/ HTTP/1.1\r\n\r\n");
	URIs.push_back("GET tel:+1-816-555-1212 HTTP/1.1\r\n\r\n");
	URIs.push_back("POST mailto:John.Doe@example.com HTTP/1.1\r\n\r\n");
	URIs.push_back("POST mailto:John.Doe@example.com?Hola=Mundo HTTP/1.1\r\n\r\n");
	URIs.push_back("POST mailto:John.Doe@example.com?Hola=Mundo#Adios HTTP/1.1\r\n\r\n");
	URIs.push_back("DELETE urn:oasis:names:specification:docbook:dtd:xml:4.1.2 HTTP/1.1\r\n\r\n");

	std::string request =	
		"GET https://username@ex%ffample.com:8080/path/to/resource?query1=value1&query2=value2#fragment HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"User-Agent: TestAgent/1.0\r\n"
		"Accept: text/html, application/xhtml+xml, application/xml;q=0.9, */*;q=0.8\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n"
		"Accept-Encoding: gzip, deflate, br\r\n"
		"Authorization: Basic dXNlcm5hbWU6cGFzc3dvcmQ=\r\n"
		"Cache-Control: no-cache\r\n"
		"Connection: keep-alive\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: 123\r\n"
		"Date: Tue, 16 Jan 2025 12:45:00 GMT\r\n"
		"Expect: 100-continue\r\n"
		"If-Match: \"etag12345\"\r\n"
		"If-Modified-Since: Mon, 15 Jan 2025 11:30:00 GMT\r\n"
		"If-None-Match: \"etag67890\"\r\n"
		"If-Range: Wed, 15 Jan 2025 10:00:00 GMT\r\n"
		"If-Unmodified-Since: Thu, 14 Jan 2025 09:00:00 GMT\r\n"
		"Max-Forwards: 10\r\n"
		"Pragma: no-cache\r\n"
		"Range: bytes=0-499\r\n"
		"Referer: https://example.com/previous\r\n"
		"TE: gzip, deflate\r\n"
		"Upgrade: HTTP/2.0\r\n"
		"Via: 1.1 example-proxy\r\n"
		"Warning: 110 example.com \"Response is stale\"\r\n"
		"\r\n";

    std::string input;
	std::string total = "";

    // Bucle interactivo
    std::cout << "HTTP Request Parser. Escribe líneas o una petición completa (Ctrl+D para salir):" << std::endl;
	int i = -1;
	while (++i < URIs.size())
    {
        //std::cout << "> ";
        //if (!std::getline(std::cin, input)) {
        //    std::cout << "\nFinalizando el programa." << std::endl;
        //    break; // Salimos si no hay más entrada (Ctrl+D en Linux/Mac o Ctrl+Z en Windows)
        //}

		total += input;
		//switch (regex.StaticMatch(total, 0))
		//switch (regex.StaticMatch(request, 0))
		switch (regex.StaticMatch(URIs[i], 0))
		{
			case REQUEST_ERROR:		std::cout << "Invalid" << std::endl << std::endl;	break;
			case REQUEST_SUCCESS:	std::cout << "Finish" << std::endl << std::endl;	break;
			case REQUEST_CONTINUE:	std::cout << "Continue" << std::endl << std::endl;	break;
		}
    }
    return 0;
}