개발 목표:
unity singleplay game을 multiplay game으로 만들어보자.

개발 환경: 
- c++(visual studio 2019)
- docker(redis - hiredis, mysql)
- socket programming ( udp, iocp server)
- unity

참고한 자료 :
- 게임서버 프로그래밍 교과서
- https://learn.unity.com/project/tanks-tutorial

구현한 내용:
db를 활용한 로그인,
(포탄처리, 움직임 처리(udp)) 동기화,
유저목록 로비, 게임 도중 접속 구현 (redis)




