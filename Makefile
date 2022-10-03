 PROJECTNAME=$(shell basename "$(PWD)")


help: Makefile
	@echo " Choose a command run in "$(PROJECTNAME)":"
	@sed -n 's/^##//p' $< | column -t -s ':' |  sed -e 's/^/ /'


## build: Build docker image
build:
	@docker build -t static-server .


## rebuild: Rebuild and restart
rebuild:
	@docker stop static-server
	@docker rmi static-server
	@docker build -t static-server .
	@make run


## run: Run server in docker on port 80
run:
	@docker run -d --rm -p 80:80 -v $(PWD)/http-test-suite/httptest:/app/httptest:ro --name static-server static-server


## rerun: Rerun
rerun:
	@docker stop static-server
	@make run


## run-test: Start httptest
func:
	@./http-test-suite/httptest.py


## test-perf: Test performance
perf:
	@wrk -t12 -c400 http://localhost:80/http-test-suite/httptest/wikipedia_russia.html


## build-nginx: Build nginx server in docker
build-nginx:
	@docker build -t nginx ./nginx-test


## run-nginx: Run nginx server in docker on 8888 port
run-nginx:
	@docker run -d --rm -p 8888:8888 --name nginx -t nginx


## test-perf-nginx: Test performance nginx
perf-nginx:
	@wrk -t12 -c400 http://localhost:8888/nginx-test/httptest/wikipedia_russia.html
