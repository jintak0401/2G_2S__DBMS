

---------------------------------------------------------    struct    ---------------------------------------------------------

<br/>
<br/>

- struct record 
  - value : record 의 값을 저장한다. 이 코드에서는 key 와 같은 값을 가진다.

<br/>
<br/>

- struct node 
  - pointers : internal node라면 (num_keys+1)개의 자식노드를 가리키는 배열이다. leaf 라면 num_keys개의 record 와 다음 leaf를 가리키는 배열이다. 
  - keys : 각 노드들에 들어있는 keys값을 저장하는 배열이다. (order - 1)개까지 가질 수 있다.
  - parent : 해당 노드들(internal node/leaf)의 부모노드를 가리키는 포인터이다. root 라면 NULL 을 가리킨다.
  - is_leaf : leaf 이면 true 를, 아니라면 false 를 가지는 bool 변수이다.
  - num_keys : 현재 가지고 있는 키값들의 개수이다. keys와 마찬가지로 (order - 1)까지 가능하다.
  - next : 각 노드들의 다음노드를 카리키는 포인터이다. print_tree 를 위한 queue 에서만 사용된다.

<br/>
<br/>

---------------------------------------------------------   function   ---------------------------------------------------------

<br/>
<br/>

* usage_1/2/3 :   
  맨 처음 및 ? 를 입력했을 때 사용법을 출력해준다.    
<br/>
<br/>
  
* enqueue : [print_tree 에 이용됨]  
print_tree 를 위한 함수로 node* 를 인자로 받아서 queue에 쌓는다.    
<br/>
<br/>

* dequeue : [print_tree 에 이용됨]   
  print_tree 를 위한 함수로 queue 의 가장 처음부분의 노드를 포인터형식으로 반환한다.    

<br/>
<br/>






* print_leaves :   
  <0번째 leaf> (0번째 record 주소) 0번째 key 값 (1번째 record 주소) 1번째 key 값 ... (1번째 leaf 주소) |   
  <1번째 leaf> (0번째 record 주소) 0번째 key 값 (1번째 record 주소) 1번째 key 값 ... (2번째 leaf 주소) | ...  ... 0    
  위와 같이 가장 왼쪽의 leaf 부터 가장 오른쪽의 leaf 까지 verbose_output 이 true 라면 괄호 안의 값과 함께, false 라면 key 값만 출력해준다.    
  출력해주는 값들은 각 노드들의 num_key 만큼의 record 주소와 그에 해당하는 key값, 마지막으로 pointers 의 가장 마지막에 들어있는 다음 leaf의 주소이다. 마지막이 0 인 이유는 가장 오른쪽의 leaf 의 마지막 pointers 가 가리키는 것이 NULL 이기 때문이다.

<br/>
<br/>

* height :   
  (root 의 높이 = 0) 을 기준으로 leaf 의 높이를 반환한다.
  
<br/>
<br/>

* path_to_root :   
  node* 인 child 를 인자로 받아, 해당 child 부터 root 까지의 길이(= 높이)를 반환한다.

<br/>
<br/>

* print_tree : [enqueue, dequeue, path_to_root 를 이용함]  
  height = 0 인 root 부터 leaf 의 height 까지 각 높이의 node 들을 왼쪽부터 오른쪽의 순서로 enqueue 및 dequeue 해주며 각 노드들이 가지고 있는 값들을 출력해준다.  
  ( (root 의 주소) ) (height = 1 의 0번째 노드주소-->A) 1번째 key 값 (height = 1 의 1번째 노드주소-->B) 2번째 key 값 ... (height = 1 의 root->num_keys번째 노드주소) |  
  ( (A 의 주소) )  (height = 2 의 0번째 노드주소) 1번째 key 값 ... (height = 2 의 A->num_keys번째 노드주소) | ( (B 의 주소) ) ...  ...|  
  ...  ...  
  ( (0 번째 leaf 주소) ) (해당 leaf 의 0번째 record 주소) 0번째 key 값 ... (1 번째 leaf 주소) | ( (1 번째 leaf 주소) ) (해당 leaf 의 0번째 record 주소) 번째 key 값 ...  ... 0  
  출력은 위와 같이 된다. 가장 마지막에 NULL 값인 0 이 오는 이유는 print_leaves 와 같으며, leaf 를 제외한 노드들의 key 값이 0번째가 아닌 1번째 부터 시작하는 이유는, 같은 key 값을 가지는 leaf 의 index 가 오른쪽에 오기 때문이다.  
  만약 tree 가 비어있다면 "Empty tree" 를 출력한다.

<br/>
<br/>

* find_and_print : [find 를 이용함]  
  key 값을 인자로 받아 find 함수를 이용하여 해당 key 값을 가진 record 를 찾고 없다면 찾을 수 없다는 메세지를 출력하고, 찾았다면 그 record 의 주소와 key 값, value 를 출력한다.

<br/>
<br/>

* find_and_print_range : [find_range 를 이용함]  
  해당 범위 key 개수 크기의 배열(예를들어 1 ~ 5 라면 5개, 4 ~ 6 이라면 3개)을 만든 후, find_range 함수를 이용하여 그 범위에 해당하는 record 의 주소와 개수를 얻어, 그 개수만큼 key값과 주소, value 를 출력한다. 만약 해당범위의 key 값을 찾지 못했다면 "None found"를 출력한다.

<br/>
<br/>

* find_range : [find_and_print_range 에 이용됨 / find_leaf 를 이용함]  
  해당 범위의 key 값을 갖는 leaf 를 find_leaf 를 통해 구한 뒤, returned_keys 와 returned_pointers 에 각각 key 값과 record 의 주소를 넣은 뒤 개수를 반환한다.

<br/>
<br/>

* find_leaf : [find_range, insert 에 이용됨]  
  인자로 받는 key 값이 있을 수 있는 leaf 를 찾아가는 함수로, root 부터 해당 leaf 를 찾아가며 verbose_output 가 true 일 경우  
  [key1  key2  ...] n  ->  
  ...  
  Leaf [key1  key2  ...] ->   
  와 같이 경로를 출력한다. 만약 인자로 받은 root 가 NULL 일 경우 "Empty tree" 를 출력한다. 마지막에 해당 leaf 를 반환한다.

<br/>
<br/>

* find : [find_and_print, insert 에 이용됨 / find_leaf 를 이용함]  
  find_leaf 를 이용하여 인자로 받은 key 와 일치하는 key 값을 가지는 record 를 찾아 반환한다. 만약 없다면 NULL 을 반환한다.

<br/>
<br/>

* cut : [insert_into_leaf_after_splitting, insert_into_node_after_splitting 에 이용됨]  
  split 을 위한 함수로 인자로 받은 length 가 짝수이면 절반을, 홀수이면 절반의 올림을 반환한다.

<br/>
<br/>

---------------------------------------------------------   Insertion   ---------------------------------------------------------

<br/>
<br/>
* make_record :   
  인자로 받는 value 를 가지는 record 를 만들어 반환한다. 만들 수 없다면 에러메시지와 함께 프로그램을 종료한다. 

<br/>
<br/>

* make_node : [make_leaf, insert_into_node_after_splitting 에 이용됨]  
  leaf 를 포함한 모든 노드를 생성하는 함수이다. 새 노드를 생성 후, (order - 1) 크기의 keys, order 크기의 pointers 를 할당하며, 이 중 하나라도 안될시 에러메세지 출력과 함께 프로그램을 종료한다. 또, is_leaf = false, num_keys=0, parent = NULL, next = NULL 로 설정하고 해당 노드를 반환한다.

<br/>
<br/>

* make_leaf : [insert_into_leaf_after_splitting, start_new_tree 에 이용됨 / make_node 를 이용함]  
  make_node 를 통해 생성한 노드의 is_leaf 를 true 로 바꾼 후 반환한다.

<br/>
<br/>

* get_left_index :   
  인자로 받는 left 를 가리키는 부모노드의 pointers 배열의 인덱스를 반환한다.

<br/>
<br/>

* insert_into_leaf :   
  인자로 받은 key 값이 들어갈 수 있는 자리를 leaf 에서 찾은 후, record 가 들어갈 수 있도록 해당 자리부터 key 값들과 pointers 들을 한 칸씩 뒤로 물린다. 그 후, 그 leaf 를 반환한다.  
  단, 이 함수 내에서 해당 자리에 key 값과 pointer 를 넣지는 않는다.

<br/>
<br/>

* insert_into_leaf_after_splitting : [make_leaf, cut, insert_into_parent 를 이용함]  
  order 개의 int 와 void* 를 받을 수 있는 temp_keys, temp_pointers 배열을 만든다. 인자로 받은 key 값을 temp_keys 와 temp_pointers의 올바른 자리에 들어갈 수 있도록 해당 자리부터 (num_keys - 1)번째의 leaf->keys 와 leaf->pointers 를 한칸씩 물린 뒤, 해당자리에 key 와 pointer 와 함께 넣어준다.  
  기존 leaf 의 오른쪽에 들어갈 새로운 new_leaf 를 만든 뒤, temp_keys 와 temp_pointers 의 약 절반씩을 각각 leaf 와 new_leaf 에 넣어준 뒤, leaf 의 마지막 pointers 는 new_leaf 를, new_leaf 의 마지막 pointers 는 기존 leaf 의 다음 leaf 를 가리키게 한 후, new_leaf 의 index 를 부모 노드에 넣을 수 있도록 insert_into_parent 함수를 호출한다.

<br/>
<br/>

* insert_into_node :   
  인자로 받은 key 와 right 가 올바른 자리에 들어갈 수 있도록, 해당자리부터 num_keys 번째 자리까지 한 칸씩 뒤로 물린 후, pointers 와 keys 의 해당자리에 key 와 right 를 넣어준다. 
  값이 하나 더 들어갔으므로 num_keys 를 1 증가시킨 후, root 를 반환한다.

<br/>
<br/>

* insert_into_node_after_splitting : [make_node, cut, insert_into_parent 를 이용함]  
  leaf 가 아닌 internal node 에 insert 한 다는 점이 insert_into_leaf_after_splitting 과 다르지만 동작 원리는 비슷하다. 단, internal node 의 마지막 pointers 는 다음 노드를 가리킬 필요가 없으므로, 해당 연산은 수행하지 않는다. split 의 결과가 parent 에도 반영될 수 있도록 insert_int_parent 를 호출한다.

<br/>
<br/>

* insert_into_parent : [insert_into_leaf/node_after_splitting 에 이용됨 / insert_into_new_root, get_left_index, insert_into_node(\_after_splitting) 을 이용함]  
  해당 노드의 부모가 없는 상태, 즉 해당 노드가 root 라면 insert_into_new_root 를 호출한다.  
  부모노드의 keys 와 pointers 가 꽉 차지 않은 상태라면 insert_into_node 를 호출한다.  
  부모노드의 keys 와 pointers 가 꽉 차서 split 이 필요한 경우 insert_into_node_after_splitting 을 호출한다.  
  결국 마지막에는 root 를 반환한다.

<br/>
<br/>

* insert_into_new_root : [make_node 를 이용함]  
  새로운 루트를 만드는 함수로, make_node 를 통해 새 노드를 생성한 후, 인자로 받은 key 를 1번째 keys 에 넣고, 인자로 받은 left 와 right 를 각각 0 번째, 1 번째 pointers 에 넣는다.   
  그리고 left 와 right 의 parent 를 새로 생성한 root 로 할당하고, 이 root 의 num_keys 를 1 로 바꿔주고 반환한다.

<br/>
<br/>

* start_new_tree : [make_leaf 를 이용함]  
  맨 처음 tree 에 insert 를 할 때 호출되는 함수로 인자로 받은 key 와 record 의 주소를 각각 keys 와 pointers 에 넣은 leaf 형태의 노드로 root 를 만들어 반환한다.

<br/>
<br/>

* insert : [find(\_leaf), make_record, start_new_tree, insert_into_leaf(\_after_splitting) 을 이용함]  
  Insertion 과 관련된 최상위 함수이다.  
  인자로 받은 key 와 일치하는 key 가 이미 존재한다, insert 하지 않고 root 를 반환한다.  
  root 가 존재하지 않는, 즉 맨 처음 tree 에 insert 를 하는 상황이라면 start_new_tree 를 호출한다.  
  find_leaf 를 통해 insert 해야하는 leaf 를 찾은 후, split 없이 들어갈 수 있다면 insert_into_leaf 를 호출한다.  
  만약 split 이 필요하다면 insert_into_leaf_after_splitting 을 호출한다.  
  결국 마지막에는 root 를 반환한다.  

<br/>
<br/>




---------------------------------------------------------   Deletion   ---------------------------------------------------------

<br/>
<br/>
* get_neighbor_index :   
  인자로 받은 노드가 부모노드의 pointers 의 n 번째에 있다면 (n-1) 을 반환한다.

<br/>
<br/>

* remove_entry_from_node :   
  인자로 받은 key 값과 일치하는 위치부터 오른쪽의 값을 한칸씩 땡겨오고, num_keys 를 1 줄인다.  
  만약 delete 한 노드가 leaf 일 경우, 다음 leaf 를 가리키는 pointers 와 record 들을 가리키는 pointers 를 제외한 나머지에 NULL 을 할당한다.  
  leaf 가 아닌 노드들이었다면 자식노드를 할당하고 있지 않은 pointers 들에 NULL 을 할당한다.  
  그리고 delete 를 수행한 노드를 반환한다.  

<br/>
<br/>

* adjust_root :   
  인자로 받은 root 가 비어있지 않다면 그냥 해당 root 를 반환한다.  
  만약 비어있지 않고 leaf 가 아니라면 0번째 pointers 를 새 root 로 만든다.  
  비어있지 않고 leaf 라면 그냥 새로운 root 를 만든다.  
  마지막으로 root 를 반환한다.

<br/>
<br/>

* coalesce_nodes : [delete_entry 를 사용함]  
  기본적으로 delete 후 크기가 작아진 노드 n 과 그 왼쪽의 노드(= neighbor)를 합치는 함수이나, n 이 가장 왼쪽의 노드라면 그 오른쪽의 노드와 합친다.  
  A. n 과 neighbor 가 leaf 가 아니라면 neighbor->keys 의 비어있는 뒷 칸에, parent 의 pointers 가 가리키는 n 과 neighbor 사이의 key 값과 n->keys 값 들을, neighbor->pointers 의 뒷 칸에 n->pointers 들을 넣는다. 그리고 모든 neighbor 의 자식 노드들의 부모 노드가 neighbor 가 되도록 만든다.  
  B. n 과 neighbor 가 leaf 라면 n 의 keys 와 pointers 들을 모두 neighbor 에 옮긴 뒤, n 의 다음 노드를 neighbor 가 가리키도록 한다.   
  A 와 B 의 경우 모두, 부모 노드가 n 과 neighbor 의 합병을 반영하도록 delete_entry 를 호출한다. 그리고 root 를 반환한다.

<br/>
<br/>

* redistribute_nodes :   
  노드 n 에 가장 가까운 neighbor 의 keys 값과 pointers 값을 하나씩 n 에 넣고 그 결과를 부모 노드에 반영하는 함수이다.   
  즉, neighbor 가 n 의 왼쪽에 있다면 n 의 keys 와 pointers 값들을 한 칸씩 오른쪽으로 옮긴 후, neighbor 의 값들을 각각 n 의 0 번째들에 넣는다.  
  오른쪽에 있다면 neighbor 의 0 번째 값들을 n 의 num_keys 번째에 넣은 후, neighbor 의 값들을 왼쪽으로 한 칸씩 옮긴다.   
  그 후, n 과 neighbor 의 num_keys 를 각각 1씩 증가, 감소 시킨 후 그 결과를 부모 노드의 keys 에 반영한다.  
  마지막으로 root 를 반환한다.

<br/>
<br/>

* delete_entry : [remove_entry_from_node, adjust_root, cut, get_neighbor_index, coalesce_nodes, redistribute_nodes 를 이용함]  
  가장 먼저 인자로 받은 key 와 일치하는 값을 delete 할 수 있도록 remove_entry_from_node 를 호출한다.  
  delete 가 수행된 노드가 root 라면 adjust_root 를 호출한다.  
  root 가 아니라면 internal node, 혹은 leaf 가 b-tree 로써 가져야하는 최소의 key 개수인지를 판별하여, 기준을 만족한다면 root 를 반환한다.  
  만족하지 못 한다면 neighbor 와, n 과 neighbor 를 가리키는 부모 노드의 pointers 사이의 keys 값을 잡아(= k_prime) Coalescence 또는 redistribute_nodes 를 호출한다.  
  neighbor 와 n 의 num_keys 를 합쳐도 허용범위 내라면 coalesce_nodes 를, 허용범위를 초과하면 redistribute_nodes 를 호출한다.

<br/>
<br/>

* delete : [find(\_leaf), delete_entry 를 이용함]  
  Deletion 과 관련된 최상위 함수이다.  
  find 와 find_leaf 를 수행하여 해당 key 값과 일치하는 record 와 leaf 가 있을 경우에만 delete_entry 를 수행한다.  

<br/>
<br/>

* destroy_tree(_nodes) :   
  재귀적으로 모든 tree 의 노드들의 메모리 동적할당을 해제하는 함수이다.

<br/>
<br/>


---------------------------------------------------------   Summary   ---------------------------------------------------------


<pre>
insert -- (이미 key 가 존재) insert 없이 root 반환  
        |  
        ㄴ(일치하는 key 없음) -- (처음으로 insert 실행) start_new_tree 호출  
                               |  
                               ㄴ(처음이 아님) -- find_leaf 호출 -- (해당 노드에 insert 가능) insert_into_leaf 호출  
                                                                  |  
                                                                  ㄴ(꽉 참) insert_into_leaf_after_splitting 호출 -- insert_into_parent 호출  
  
    
insert_into_parent -- (해당 노드가 root) insert_into_new_root 호출  
                    |  
                    ㄴ(부모 노드가 있음) get_left_index 호출 -- (부모 노드가 꽉 차지 않음) insert_into_node 호출  
                                                              |  
                                                              ㄴ(꽉 참) insert_into_node_after_splitting 호출 -- insert_into_parent 호출  
   



  
  
delete -- (해당 key 가 없음) root 반환  
        |  
        ㄴ(해당 key 있음) delete_entry 호출 -- remove_entry_from_node 호출(= key, record 삭제)  
  
  
remove_entry_from_node 호출 후 -- (해당 노드가 root) adjust_root -- (아직 key 값 남음) root 반환  
                                |                                 |  
                                |                                 ㄴ(key 값 없음) -- (root == leaf) 새 root 생성  
                                |                                                  |  
                                |                                                  ㄴ(root != leaf) root->pointers[0] 을 새 root 로  
                                |  
                                ㄴ(root 가 아님) -- (key 개수 >= 최소개수) -- root 반환  
                                                  |  
                                                  ㄴ(허용범위 밖) -- (neighbor, n 의 key 개수 합 < 허용범위) coalesce_nodes 호출(= neighbor 와 n 합병)  
                                                                   |  
                                                                   ㄴ(neighbor, n 의 key 개수 합 >= 허용범위) redistribute_nodes 호출(= keys, pointers 하나씩 neighbor -> n)  
</pre>



---------------------------------------------------------   How to On-Disk   ---------------------------------------------------------


<br/>
<br/>

| 현재 | 변경 후 |
| --- | --- |
| 프로그램 종료시 tree 가 삭제된다 | 종료 후에도 유지될 수 있도록 저장해야 한다 |
| node* 인 root 를 통해 search, insert, delete 가 이루어진다 | File Manager 를 거치도록 한다 |
| tree 의 전반적인 정보를 알기 힘들다 | 해당 파일의 전반적인 정보를 담은 header page 를 추가한다 |
| leaf node 가 record 의 포인터를 배열로 갖는다 | leaf node 가 record 자체를 배열로 갖는다 |
<br/>
<br/>
