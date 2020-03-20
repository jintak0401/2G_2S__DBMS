<br/><br/>
[1] recovery <br/><br/>
init_db 을 호출할 때 recovery 라는 함수가 호출됩니다. 즉, 맨 처음 단 한 번만 호출됩니다.<br/>
recovery 함수에서 log_file 로그의 맨 처음부터 consider_redo 를 통해 page_LSN 과 record의 value 를 변경해갑니다. <br/>
마지막 log까지 winner, loser에 관계없이 모든 로그에 대해 consider_redo를 하고, 끝까지 다 했다면 undo를 진행합니다. <br/>
undo는 abort 함수를 통해 진행됩니다.(abort_trx에서도 호출되는 함수입니다!) <br/>
<br/>


<br/>
[2] abort <br/><br/>
abort 함수에서는 undo를 진행합니다. <br/>
CLR의 NextUndoLSN 은 prev_LSN에 적으며, 뒤에서부터 거슬러 올라가며 undo를 진행합니다. <br/>
모두 진행한 후 type이 ABORT 인 로그를 발급하여 log_tail에 적습니다. <br/>
그리고 WAL에 의해 COMMIT 할 때 로그를 적어 내리는 것과 마찬가지로, ABORT 로그를 발급하고 CLR들을 모두 로그파일에 적어내립니다. <br/>
<br/>


<br/>
[3] make_log <br/><br/>
make_log 함수에서 로그를 발급합니다. <br/>
여러 트랜잭션들이 동시에 접근하여 로그를 발급할 수도 있으므로 pthread_mutex_t log_mutex 를 통해 중복된 LSN이 없도록 래치를 구현했습니다. <br/>
<br/>


<br/>
[4] flush_log <br/><br/>
flush_log 함수를 통해 log_tail 를 파일로 적어내립니다. <br/>
단, 이 함수 또한 여러 트랜잭션들에 의해 동시에 호출되어 파일에 적어내릴 가능성이 있습니다.<br/>
그래서 이 함수 내에서도 pthread_mutex_t flush_mutex 를 통해 래치를 구현했습니다.<br/>
<br/>

