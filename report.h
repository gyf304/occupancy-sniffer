const char* report_tmpl = 
"POST " " HTTP/1.0\r\n"
"Host: " REPORT_HOST "\r\n"
"User-Agent: ESP8266\r\n"
"Content-Type: application/json\r\n"
"Connection: close\r\n"
"Content-Length: %d\n\n"
