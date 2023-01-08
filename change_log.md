change log
====
# 1.6.2
* add numa memory manager

# 1.6.1

* Fix CheckHealth not set has_request_code
* Deliver timeout from client to server
* Fix a bug that server will send unexpected data frame to client if there are errors occur during processing stream create request
* Fix LA selection runs too long
* Fix HttpResponse error
* Fix client side retry policy
* Support parse proto-text format http request body
* Support dump and replay for HTTP protocol
* Fix rpc_press can't send request equably
* Fix discovery naming service core
* Fix the first fiber keytable
* Fix coredump cause by bad growth_non_responsive http request