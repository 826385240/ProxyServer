#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "parent_process.h"
#include "../common/common.h"
#include "../unix_domain/unix_domain.h"

int do_parent(unix_domain& ud)
{
	ud.set_proc_type(PROC_TYPE_PARENT);

	//��ʼ���׽���
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	CHECK_ERROR(sockfd)

	//��ʼ���������׽��ֵ�ַ�ṹ
	sockaddr_in serveaddr;
	serveaddr.sin_family = AF_INET;
	serveaddr.sin_port = htons(SERVER_PORT);
	serveaddr.sin_addr.s_addr = INADDR_ANY;
	//inet_pton(AF_INET , serverIP , &serveaddr.sin_addr);

	//�����׽��ֵ�ַ����ѡ��
	int flag = 1, len = sizeof(flag);
	int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, len);
	CHECK_ERROR(ret)

	//���׽��ֵ�ַ�ṹ���׽���
	ret = bind(sockfd, (sockaddr*)&serveaddr, sizeof(serveaddr));
	CHECK_ERROR(ret)

	//������������
	ret = listen(sockfd, 0);
	CHECK_ERROR(ret)

	//�����µ�����
	sockaddr_in new_addr;
	socklen_t new_addr_size = sizeof(new_addr);

	while (true)
	{
		int new_fd = accept(sockfd, (sockaddr*)&new_addr, &new_addr_size);
		CHECK_ERROR(new_fd)

		ud.send_fd((void*)"", 1, new_fd);
		close(new_fd);
	}

	return 0;
}