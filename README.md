## HighLoad Static Server

### About 

The server is written on C++ language using prefork + polling technology

### Functionality testing

Status: <ins>All tests successfully passed</ins>

```

test_directory_index (__main__.HttpServer)
directory index file exists ... ok
test_document_root_escaping (__main__.HttpServer)
document root escaping forbidden ... ok
test_empty_request (__main__.HttpServer)
Send empty line ... ok
test_file_in_nested_folders (__main__.HttpServer)
file located in nested folders ... ok
test_file_not_found (__main__.HttpServer)
absent file returns 404 ... ok
test_file_type_css (__main__.HttpServer)
Content-Type for .css ... ok
test_file_type_gif (__main__.HttpServer)
Content-Type for .gif ... ok
test_file_type_html (__main__.HttpServer)
Content-Type for .html ... ok
test_file_type_jpeg (__main__.HttpServer)
Content-Type for .jpeg ... ok
test_file_type_jpg (__main__.HttpServer)
Content-Type for .jpg ... ok
test_file_type_js (__main__.HttpServer)
Content-Type for .js ... ok
test_file_type_png (__main__.HttpServer)
Content-Type for .png ... ok
test_file_type_swf (__main__.HttpServer)
Content-Type for .swf ... ok
test_file_urlencoded (__main__.HttpServer)
urlencoded filename ... ok
test_file_with_dot_in_name (__main__.HttpServer)
file with two dots in name ... ok
test_file_with_query_string (__main__.HttpServer)
query string with get params ... ok
test_file_with_slash_after_filename (__main__.HttpServer)
slash after filename ... ok
test_file_with_spaces (__main__.HttpServer)
filename with spaces ... ok
test_head_method (__main__.HttpServer)
head method support ... ok
test_index_not_found (__main__.HttpServer)
directory index file absent ... ok
test_large_file (__main__.HttpServer)
large file downloaded correctly ... ok
test_post_method (__main__.HttpServer)
post method forbidden ... ok
test_request_without_two_newlines (__main__.HttpServer)
Send GET without to newlines ... ok
test_server_header (__main__.HttpServer)
Server header exists ... ok

----------------------------------------------------------------------
Ran 24 tests in 7.665s

OK

```

### Performance testing

<ins>Description</ins>: <br/>

Load testing was carried out with a comparison of characteristics 
with a high-performance web server for returning statics - Nginx. 
The results of the comparative analysis are presented below.

The tests were run several times (in this experiment, we will take the number equal to 3) on different file types and the average results are shown below.

Список файлов для тестирования:

1) wikipedia_russia.html
2) splash.css

| **File type for testing** | **Own Static Server**, rps | **Nginx**, rps | **Number of times difference** |
|---------------------------|----------------------------|----------------|--------------------------------|
|   wikipedia_russia.html   | 11100                      | 29975.20       | 2.70                           |
|        splash.css         | 15142                      | 30120.40       | 2.09                           |

