# 回退N协议

## 对于发送方

一个二维数组做缓冲区，从网络层获取大量的数据，把缓冲区填满

同时也是获取一个包，成帧之后就送到物理层，但是，没有收到ACK之前一直在缓冲区。
接收到ACK后，清空一个缓冲区

如果缓冲区超级大
那么就超级大挺好的。就是保持一直有帧发送。

如果缓冲区小
那么当我缓冲区填满了，第一个ACK还没到手，所以disable network

如果缓冲区为1
那么没发送出去之前，不能enable network

如果超时orNAK
那么重传。


## 对于接收方(接受窗口大小为1)

之所以frame_expect是刚刚拿走的那一帧的seq，是因为ACK有可能丢了

接受方可能遇到问题
1. 收到帧是坏的
2. 收到的帧不是期待的
3. 收到的帧是ACK或者NAK

如果收到的数据不是想要的，则对后面所有的都不管
> 此处对应的是无NAK
> 然后，发送方由于没有收到ACK，所以挨个超时，然后挨个重传。
> 可以不用等待挨个超时，首个超时之后就直接重传后面的所有。

如果收到的数据不是想要的，则发送NAK
>　然后发送方收到了NAK，则停止计时器，然后重传 
>>  若一个NAK丢了，在直接超时的情况下，后面的就全重传了，所以只发送一个NAK就可以，丢了就等待超时

对于使用捎带确认
NAK 不能使用捎带确认？
NAK应该越早到越好，所以就应该是单独的帧，否则就要等发送方超时之后才知道自己坏了，多发了好多无效帧。


