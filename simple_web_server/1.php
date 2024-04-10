<?php
phpinfo();
echo "111";

var_dump($_POST);

$body  = file_get_contents('php://input');
var_dump($body);
