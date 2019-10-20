package metrics

// ObjectPool is an minimalistic object pool to avoid
// any circular dependencies on any other object pool.
type ObjectPool struct {
	values chan interface{}
	alloc  func() interface{}
}

// NewObjectPool creates a new pool.
func NewObjectPool(size int) *ObjectPool {
	return &ObjectPool{
		values: make(chan interface{}, size),
	}
}

// Init initializes the object pool.
func (p *ObjectPool) Init(alloc func() interface{}) {
	p.alloc = alloc

	for i := 0; i < cap(p.values); i++ {
		p.values <- p.alloc()
	}
}

// Get gets an object from the pool.
func (p *ObjectPool) Get() interface{} {
	var v interface{}
	select {
	case v = <-p.values:
	default:
		v = p.alloc()
	}
	return v
}

// Put puts an object back to the pool.
func (p *ObjectPool) Put(obj interface{}) {
	select {
	case p.values <- obj:
	default:
	}
}
