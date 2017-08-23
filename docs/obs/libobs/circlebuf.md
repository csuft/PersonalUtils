# OBS Studio源码走读之动态循环缓冲区(circlebuf)

动态循环缓冲区结构定义如下：

```c
struct circlebuf {
	void   *data;
	size_t size;

	size_t start_pos;
	size_t end_pos;
	size_t capacity;
};
```

