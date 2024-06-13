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
* $ackQueueCounter$ - liczba akceptacji dołączeni do kolejki oczekiwania na grupę
* $groupQueue$ - wektor par $(id, lamport)$ procesów oczekujących na grupę
* $groupMembers$ - lista $id$ procesów w grupie
* $ackResCounter$ - liczba akceptacji dołączeni do kolejki oczekiwania na zasób
* $resQueue$ - wektor par $(id, lamport)$ liderów oczekujących na zasób
* $breakProb$ - prawdopodobieństwo wymuszenia przerwy od ubiegania się o zasób
* $breakTime$ - czas przerwy w sekundach


## Używane komunikaty
* $REQGROUP$ - prośba o dołączenie do kolejki procesów oczekujących na grupę
* $ACKGROUP$ - udzielonie zgody na dołączenie do kolejki procesów oczekujących na grupę
* $GROUPFORMED$ - wiadomość od lidera grupy zawierająca listę $groupMembers$ procesów w grupie
* $REQRES$ - prośba o dołącznie do kolejki leaderów oczekujących na zasób
* $ACKRES$ - udzielonie zgody na dołącznie do kolejki leaderów oczekujących na zasób
* $START$ - rozpoczęcie korzystania z zasobu, zawiera czas trawia dostępu do zasobu
* $END$ - zakończenie korzystania z zasobu

Początkowo procesy znają wartości $P$, $G$, $T$ oraz $id$ wszystkich pozostałych procesów.


## Działanie algorytmu
### 1. Dobieranie się w grupy (procesy które dobierają się w grupę)
1. Początkowo każdy proces ubiegający się o zasób wysyła komunikat $REQGROUP$
2. Proces zlicza otrzymane $ACKGROUP$ w $ackQueueCounter$. Gdy $ackQueueCounter = T-1$ proces dodaje się do słownika $groupQueue$
2. Proces reaguje na $REQGROUP$ odsyłając $ACKGROUP$ oraz dodając nadawcę do słownika $groupQueue$
3. Proces sprawdza czy w $groupQueue$ znajduje się $G$ procesów. Jeżeli tak to proces o najmniejszym zegarze lamporta zostaje liderem
4. Jeżeli proces jest liderem to: 
    1. Wypełnia listę $groupMembers$ kolejnymi $G$ procesami z najmniejszymi zegarami lamporta
    2. Wysyła do wszystkich procesów komunikat $GROUPFORMED$ wraz z listą $groupMembers$
    3. Poces usuwa ze słownika $groupQueue$ procesy z listy $groupMembers$
5. Jeżeli proces nie jest liderem to:
    1. Oczekuje na komunikat $GROUPFORMED$
    2. Jeżeli id procesu znajduje się w liście $groupMembers$ przesłanej w komunikacie $GROUPFORMED$ wypełnia swoją listę $groupMembers$ procesami z komunikatu
    4. Poces usuwa ze słownika $groupQueue$ procesy z listy $groupMembers$
6. Jeżeli proces dostał komunikat $GROUPFORMED$, ale nie znajduje się w liście $groupMembers$ przesłanej w komunikacie to sprawdza czy nie zostaje liderem pozostałych procesów

   



### 2. Zarządzanie zasobem (procesy które dobrały się w grupę)
1. Jeżeli proces jest liderem to:
    1. Wysyła do wszystkich procesów komunikat $REQRES$
    2. Proces zlicza otrzymane $ACKRES$ w $ackResCounter$. Gdy $ackResCounter = T-1$ proces dodaje się do słownika $resQueue$
    4. Jeżeli proces mieści się w grupie P procesów o najmniejszych wartościach zegara lamporta to wysyła komunikat $START$ do procesów z listy $groupMembers$
3. Procesy reagujż na $REQRES$ dodając nadawcę do słownika $resQueue$ oraz odsyłając $ACKRES$
4. Jeżeli proces nie jest liderem to oczekuje na komunikat $START$, po którym rozpoczyna korzystanie z zasobu


### 3. Zakończenie korzystania z zasobu (procesy które zakończyły korzystanie z zasobu)
1. Jeżeli proces jest liderem to: 
    1. Wysyła komunikat $END$ do wszystkich procesów
    2. Procesy reagują na komunikat $END$ usuwając $id$ nadawcy z listy $resQueue$
2. Proces zeruje swoją listę $groupMembers$ oraz licznik $ackQueueCounter$ i $ackResCounter$
3. Proces losuje z prawdopodobieństwem $breakProb$ to czy zostanie na niego nałożona przerwa. Jeżeli tak, to musi odczekać $breakTime$ ms.
4. Proces ponownie rozpoczyna porces dobierania się w grupy