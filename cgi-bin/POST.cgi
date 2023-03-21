import os
import sys
import string

root_path = ""

def import_httpd_conf():
    with open("../conf/httpd.conf", "r") as fd:
        lines = fd.readlines()
        global root_path
        root_path = lines[1].strip()
        # print("snd line: ", root_path)

def get_content_type(file_path) -> string:
    # print(file_path.split("."))
    file_ext = file_path.split(".")[3]

    if file_ext == "html" or file_ext == "htm":
        return "text/html"
    elif file_ext == "jpeg" or file_ext == "jpg":
        return "image/jpeg"
    elif file_ext == "gif":
        return "image/gif"
    else:
        return "text/plain"


def send_http_header(code, content_type, content_length) -> None:
    if code == 200:
        print("HTTP/1.0 200 OK\r\n", end="")
    elif code == 404:
        print("HTTP/1.0 404 Not Found\r\n", end="")
    
    print("Content-Type: " + str(content_type) + "\r\n", end ="")
    print("Content-Length: " + str(content_length) + "\r\n", end ="")
    print("Connection: close\r\n", end="")
    print("Server: LiteSpeed\r\n", end="")
    print("Date: Sat, 28 Nov 2009 04:36:25 GMT\r\n", end="")	
    print("Last-Modified: Mon, 10 April 2004 09:26:15 GMT\r\n", end="")
    print("Expires: Mon, 22 Jan 2010 01:00:00 GMT\r\n", end="")
    print("\r\n", end="")

def main():
    import_httpd_conf()

    argc = len(sys.argv)
    
    args = sys.argv[1].split("&")
    first_in = args[0].split("=")[1]
    second_in = args[1].split("=")[1]
    
    out = "POST inputs were: " + first_in + " and " + second_in
    send_http_header(200, "text/plain", len(out))
    # sent plain text output containing post inputs
    print(out)
    

if __name__ == "__main__":
    main()

