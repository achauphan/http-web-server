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
    # print("argc: " + str(argc))
    # print(sys.argv)
    
    file_path = sys.argv[1].split("=")[1]
    file_path = root_path + "/" + file_path
    # print("new path: ", file_path)

    if not os.path.exists(file_path):
        res_body = "Error: HTTP 404 - File not found"
        send_http_header(404, "text/plain", len(res_body))
        print(res_body, end ="") # printed to client socket
    else:
        file_size = os.path.getsize(file_path)
        send_http_header(200, get_content_type(file_path), file_size)
        with open(file_path, "r") as fd:
            lines = fd.readlines()
            for line in lines:
                # python print adds stupid newline characters
                # removing them with end="" breaks sent html text newline characters
                # so I am leaving them in so that it works
                print(line)
            # for line in fd:
            #     print(line, end="")
    

if __name__ == "__main__":
    main()

