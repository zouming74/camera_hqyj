#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#define SERVER_IP "192.168.151.219" //修改为你的ip地址
#define SERVER_PORT 8080 //修改为你的端口

int main()
{
    int camfd = open("/dev/video0", O_RDWR);
    if (camfd < 0) {
        perror("cam open");
        return -1;
    }
    printf("open camera device success\n");

    struct v4l2_capability cap;
    int ret = ioctl(camfd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        perror("ioctl QUERYCAP");
        return -1;
    }
    printf("Driver Name:%s\nCard Name:%s\nBus Info:%s\nVersion:%u.%u\n", cap.driver, cap.card, cap.bus_info, cap.version, cap.capabilities);

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) || !(cap.capabilities & V4L2_CAP_STREAMING)) {
        printf("The device does not support video capture or streaming.\n");
        return -1;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 1280;
    fmt.fmt.pix.height = 720;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(camfd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("ioctl Set Format");
        return -1;
    }

    struct v4l2_requestbuffers reqbuf;
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 4;
    ret = ioctl(camfd, VIDIOC_REQBUFS, &reqbuf);
    if (ret < 0) {
        perror("ioctl Requset Buffers");
        return -1;
    }

    typedef struct {
        void* start;
        size_t length;
    } VideoBuffer;
    VideoBuffer* buffers = (VideoBuffer*)calloc(reqbuf.count, sizeof(VideoBuffer));
    if (buffers == NULL) {
        perror("calloc");
        return -1;
    }

    struct v4l2_buffer buf;
    for (unsigned int i = 0; i < reqbuf.count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ret = ioctl(camfd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            perror("ioctl QUERYBUF");
            return -1;
        }

        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, camfd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            perror("mmap");
            return -1;
        }

        ret = ioctl(camfd, VIDIOC_QBUF, &buf);
        if (ret < 0) {
            perror("ioctl QBUF");
            return -1;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(camfd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        perror("ioctl Stream On");
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    ret = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        perror("bind");
        return -1;
    }

    ret = listen(sockfd, 1);
    if (ret < 0) {
        perror("listen");
        return -1;
    }

    printf("Waiting for client connection...\n");
    int client_sockfd = accept(sockfd, NULL, NULL);
    if (client_sockfd < 0) {
        perror("accept");
        return -1;
    }
    printf("Client connected.\n");

    struct pollfd fds[1];
    fds[0].fd = camfd;
    fds[0].events = POLLIN;

    while (1) {
        ret = poll(fds, 1, 1000);
        if (ret < 0) {
            perror("poll");
            return -1;
        }

        if (fds[0].revents & POLLIN) {
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            ret = ioctl(camfd, VIDIOC_DQBUF, &buf);
            if (ret < 0) {
                perror("ioctl DQBUF");
                return -1;
            }

            int imageSize = buf.bytesused;
            ret = send(client_sockfd, &imageSize, sizeof(imageSize), 0);
            if (ret < 0) {
                perror("send imageSize");
                return -1;
            }

            ret = send(client_sockfd, buffers[buf.index].start, buf.bytesused, 0);
            if (ret < 0) {
                perror("send image data");
                return -1;
            }

            ret = ioctl(camfd, VIDIOC_QBUF, &buf);
            if (ret < 0) {
                perror("ioctl QBUF");
                return -1;
            }
        }
    }

    ret = ioctl(camfd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        perror("ioctl Stream Off");
        return -1;
    }

    for (unsigned int i = 0; i < reqbuf.count; i++) {
        ret = munmap(buffers[i].start, buffers[i].length);
        if (ret < 0) {
            perror("munmap");
            return -1;
        }
    }
    free(buffers);
    close(camfd);
    close(client_sockfd);
    close(sockfd);

    printf("success\n");
    return 0;
}
