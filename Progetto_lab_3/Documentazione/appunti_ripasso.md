1. ConfigReader.java
    A. Il tipo Properties viene popolato come una hashtable da un file.
    B. Integer.parseInt(value); converte una stringa in valore

2. ServerMain
    A. Se vuoi cambiare da dove prendo le parole cambia datasetPath

3. WordDataset.java
    A. La classe TypeToken è un trucco inventato da Google: quando compili il codice, il programma "dimentica" i tipi generici contenuti nelle liste. A runtime List<GameData> viene vista come una List<> generica.

    B. Per implementazione il programma carica TUTTE le parole in Ram subito, per velocità.
    Si può dire che è inefficiente o efficiente in base a come si guarda.

    C. Commentato

4. User.java



AL MOMENTO SONO IN SERVERMAIN -> NIOSERVER -> USER