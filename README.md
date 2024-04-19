# Safari na wildzie 
## Parametry i założenia
$P$ - liczba zasobów\
$G$ - rozmiar grupy procesów\
$T$ - liczba procesów\
$T >> P, T >= (P + 1) * G$\
Priorytet procesów jest ustalany na podstawie zegarów Lamporta, jeśli wartości zegarów są równe, większy priorytet ma proces o niższym numerze id. 

## Używane komunikaty
* $REQ$ - prośba o dostęp do zasoby\
* $ACK$ - udzielona zgoda na dostęp do zasobu. Dodatkowo $ACK$ posiada flagę $in\_group$, której wartość ustawiana jest na $True$ gdy proces do której skierowany jest komunikat należy do tej samej grupy.
* $SYNC$ - rozpoczęcie korzystania z zasobu

Za każdym razem gdy proces zamierza wysłać komunikat, najpierw aktualizuje wartość zegara Lamporta. Za każdym razem gdy proces odbiera komunikat w pierwszej kolejnośći porównuje wartości zegarów, a następnie aktualizuje własny zegar Lamporta.


## Zmienne i stałe używane przez procesy:
* $id$ - identyfikator procesu
* $ack\_counter$ - Licznik otrzymanych komunikatów $ACK$
* $ack\_list$ - Lista procesów które czekają na ich akceptację poprzez komunikat $ACK$.
* $group\_list$ - Lista procesów w aktualnej grupie wycieczkowej
* $req\_map$ - Mapę wiążącą priorytety wiadomości $REQ$ z id procesów, które wysłały wiadomości $REQ$ - optymalizacja

Początkowo procesy znają wartości $P$, $G$, $T$ oraz $id$ wszystkich pozostałych procesów.
## Działanie algorytmu
### 1. Ubieganie się o dostęp do grupy - algorytm Ricarta-Agrawali
1. Każdy proces początkowo ubiega się o dostęp do grupy. Procesy wysyłają $REQ$ do wszystkich innych procesów. 
2. Każdy proces X, który otrzyma komunikat $REQ$ od procesu Y odpowiada w następujący sposób:
    * Jeśli proces X ma wyższy priorytet niż Y, zapisuje $id$ procesu Y na liście $ack\_list$. Dodatkowo proces X inkrementuje $ack\_counter$ jeśli priorytet, z którym wysłał $REQ$ do procesu Y jest wyższy niż priorytet otrzymanej wiadomości $REQ$ od procesu Y.
    * Jeśli proces X ma niższy priorytet od procesu Y, proces X wysyła $ACK$ do procesu Y. Proces X nie musi wysyłać widomości $ACK$, jeżeli priorytet otrzymanej od procesu Y wiadomości $REQ$ jest wyższy niż priorytet, z którym proces X wysłał komunikat $REQ$ do procesu Y.
3. Jeżeli proces otrzyma komunikat $ACK$ inkrementuje $ack\_counter$.
4. Proces oczekuje, aż wartość jego $ack\_counter == T - 1$, co oznacza, że ma on dostęp do grupy. Następnie proces ustawia wartość $ack\_counter := 0$.
5. Proces wysyła komunikat $ACK$ z flagą $in\_group := true$ do pierwszych $G - 1 - len(group\_list)$ procesów ze swojej listy $ack\_list$. Proces wysyła komunikat $ACK$ z flagą $in\_group = false$ do kolejnych $G * (P - 1)$ procesów ze swojej listy $ack\_list$. Proces usuwa $id$ wszystkich procesów, do których wysłał komunikat $ACK$ ze swojej (ack_list). 
6. Proces dodaje $id$ wszystkich procesów, do których wysłał $ACK$ z flagą $in\_group = true$ do swojej $group\_list$.


## 2. Synchronizacja w grupie
1. Każdy proces X, który otrzymał komunikat ACK z flagą $in\_group = true$ od procesu Y dodaje $id$ procesu Y do swojej listy $group\_list$.
2. Jeżeli po wykonaniu punktu **1.5** zachodzi $len(group\_list) == G - 1$ to została zebrana pełna grupa. Proces wysyła komunikat $SYNC$ do procesów z listy $group\_list$ oraz rozpoczyna korzystanie z zasobu
3. Każdy proces który otrzymał komunikat $SYNC$ rozpoczyna korzystanie z zasobu.

## 3. Zakończenie korzystania z zasobu
1. Proces czyści liste $group\_list$
2. Proces który zakończył korzystanie z zasoby wysyła komunikat $REC$ do wszystkich procesów.
3. Proces wysyła komunikat $ACK$ do pozostałych $T - G * P$ procesów z listy $ack\_list$, a następnie je z niej usuwa.
