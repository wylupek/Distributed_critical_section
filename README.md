# Safari na Wildzie
## Parametry i założenia
$P$ - liczba przewodników/zasobów\
$G$ - rozmiar grupy turystów/procesów\
$T$ - liczba turystów/procesów\
$T >> P$\
$T \geq  2*G$\
Priorytet procesów jest ustalany na podstawie zegarów Lamporta, jeśli wartości zegarów są równe, większy priorytet ma proces o niższym numerze id. 


## Zmienne i stałe używane przez procesy:
* $id$ - identyfikator procesu
* $lamport$ - wartość zegara lamporta
* $ack\_group\_counter$ - liczba akceptacji dołączeni do kolejki oczekiwania na grupę
* $group\_q$ - słownik $id: lamport$ procesów oczekujących na grupę
* $leaders$ - lista $id$ procesów
* $in\_group$ - lista $id$ procesów w grupie
* $ack\_res\_counter$ - liczba akceptacji dołączeni do kolejki oczekiwania na zasób
* $res\_q$ - słownik $id: lamport$ liderów oczekujących na zasób
* $break\_prob$ - prawdopodobieństwo wymuszenia przerwy od ubiegania się o zasób
* $break\_time$ - czas przerwy w ms


## Używane komunikaty
* $REQGROUP$ - prośba o dołączenie do kolejki procesów oczekujących na grupę
* $ACKGROUP$ - udzielonie zgody na dołączenie do kolejki procesów oczekujących na grupę
* $GROUPFORMED$ - wiadomość od lidera grupy zawierająca słownik $id: lamport$ procesów w grupie oraz flagę $is\_in\_group$, która ustawiona jest na $true$ gdy odbiorca znajduje się w grupie razem z liderem
* $REQRES$ - prośba o dołącznie do kolejki leaderów oczekujących na zasób
* $ACKRES$ - udzielonie zgody na dołącznie do kolejki leaderów oczekujących na zasób
* $START$ - rozpoczęcie korzystania z zasobu, zawiera czas trawia dostępu do zasobu
* $END$ - zakończenie korzystania z zasobu

Początkowo procesy znają wartości $P$, $G$, $T$ oraz $id$ wszystkich pozostałych procesów.


## Działanie algorytmu
### 1. Dobieranie się w grupy (procesy które dobierają się w grupę)
1. Początkowo każdy proces ubiegający się o zasób wysyła komunikat $REQGROUP$
2. Proces zlicza otrzymane $ACKGROUP$ w $ack\_group\_counter$. Gdy $ack\_group\_counter = T-1$ proces dodaje się do słownika $group\_q$
2. Proces reaguje na $REQGROUP$ odsyłając $ACKGROUP$ oraz dodając nadawcę do słownika $group\_q$
3. Proces sprawdza czy w $group\_q$ znajduje się G procesów. Jeżeli tak to proces o najmniejszym zegarze lamporta zostaje liderem. 
4. Jeżeli proces jest liderem to: 
    1. Wypełnia listę $in\_group$ kolejnymi $G$ procesami z najmniejszymi zegarami lamporta
    2. Wysyła do wszystkich procesów komunikat $GROUPFORMED$ wraz z listą $in\_group$ oraz flagą odpowiednio ustawioną flagą $is\_in\_group$
    3. Dodaje się do listy $leaders$
5. Jeżeli proces nie jest liderem to:
    1. Oczekuje na komunikat $GROUPFORMED$. 
    2. Po otrzymaniu komunikatu $GROUPFORMED$ z flagą $is\_in\_group=true$ wypełnia listę $in\_group$ procesami z komunikatu.
6. Procesy reagują na komuniakt $GROUPFORMED$ niezależnie od flagi $is\_in\_group$ usuwając procesy przesłane w komunikacie ze słownika $group\_q$ oraz dodają nadawcę do listy $leaders$


### 2. Zarządzanie zasobem (procesy które dobrały się w grupę)
1. Jeżeli proces jest liderem to:
    1. Wysyła do wszystkich procesów komunikat $REQRES$
    2. Proces zlicza otrzymane $ACKRES$ w $ack\_res\_counter$. Gdy $ack\_res\_counter = len(leaders)-1$ proces dodaje się do słownika $res\_q$
    3. Proces reaguje na $REQRES$ dodając nadawcę do słownika $res\_q$ oraz odsyłając $ACKRES$ jeżeli jest w liście $leaders$ 
    4. Jeżeli proces mieści się w grupie P procesów o najmniejszych wartościach zegara lamporta to wysyła komunikat $START$ do procesów z listy $in\_group$
4. Jeżeli proces nie jest liderem to oczekuje na komunikat $START$, po którym rozpoczyna korzystanie z zasobu


### 3. Zakończenie korzystania z zasobu (procesy które zakończyły korzystanie z zasobu)
1. Jeżeli proces jest liderem to: 
    1. Wysyła komunikat $END$ do wszystkich procesów
    2. Procesy reagują na komunikat $END$ usuwając $id$ nadawcy z listy $res\_q$ oraz listy $leaders$
2. Proces zeruje swoją listę $in\_group$ oraz licznik $ack\_group\_counter$ i $ack\_res\_counter$
3. Proces losuje z prawdopodobieństwem $break\_prob$ to czy zostanie na niego nałożona przerwa. Jeżeli tak, to musi odczekać $break\_time$ ms.
4. Proces ponownie rozpoczyna porces dobierania się w grupy
