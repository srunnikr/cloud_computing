#!/bin/bash

servers=( "a" "b" "c" )
expression="/#WEB_SERVER_LIST/a\\"
for server in ${servers[@]}; do
    expression="${expression}\tserver ${server}:8080;\n"
done
echo $expression
sed "$expression" nginx.conf.template
