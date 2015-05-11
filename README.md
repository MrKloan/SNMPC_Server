SNMPC (Server)
=============

Projet libre réalisé dans le cadre de la matière *Langage C*, durant le premier semestre de 2ème année (2i) 2014-2015 à l'*École Supérieure de Génie Informatique* (ESGI).

L'objectif de la suite logicielle SNMPC est de répondre aux besoins des utilisateurs de contrôleurs Teracom TCW180B & TCW181B-CM. Bien que configurables grâce à une interface Web, cette dernière n'offre que peu de choix à l'utilisateur, n'est pas ergonomique et est unique pour chaque contrôleur.

Cependant, il est possible de les configurer via le réseau grâce au protocole SNMP, implémentant un certain nombre de commandes de contrôle. En plus de cela, nous avons pu développer un dispositif de planification de tâches.

SNMPC est un couple logiciel client/serveur : le programme serveur, hébergé sur un RaspberryPi, permettant une exécution permanente et une liaison sur un réseau local avec les contrôleurs, tandis que les programmes clients se connectent directement au serveur via le protocole TCP/IP.

En plus de sécuriser le dispositif en plaçant un composant logiciel entre l'utilisateur et le matériel, l'utilisation d'un serveur permet à l'utilisateur de s'y connecter depuis n'importe quel poste disposant d'une connexion internet.

Bibliothèques
------------

* [pthread](http://en.wikipedia.org/wiki/POSIX_Threads)
* [libxml2](http://www.xmlsoft.org/)
* [netsnmp](http://www.net-snmp.org/)

Équipe
------------
* Mathieu Boisnard
* [Valentin Fries](http://fries.io)