Аргументы: путь к файлу, количество потоков, порт, ip-адрес, последние 2 имеют деволтные значения

Использование 1)
  Из мейна запускаем сервер, форкаемся, ждем подключения
  Запускаем клиент
  
Использование 2)
  Запускаем main_server
  Запускаем main_client 
  
Подтвреждение того, что оно работает в каком-то виде:

  запускаем 2 использование
  видим, что выводится offset, размер переданного пакета данных и размер структуры, содеражщей заголовок пакета, однако, по невыясненной причине, несмотря  на то, что данные читаются из сокета в буффер корректно (и заголовок пакета и его содержимое), в файл (с помощью mmap и memcpy) они не записываются, а с помощью write записываются
  
Таким образом, осталось добавить возможность ориентации по файлу с помощью аттрибута offset 

Также необходимо решить проблему с неправильным оффсетом при использовании threadpool
