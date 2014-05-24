
#ifndef _SQL_BUF_HPP_
#define _SQL_BUF_HPP_

#include <fstream>

namespace std{

template<class charT=char> struct CDefaultReadWrite
{
	int write(const charT* s, streamsize size)
	{
		s = s;
		return size;
	}

	int read(charT* s, streamsize size)
	{
		s = s;
		return size;
	}

	int readbufsize()
	{
		return 128;
	}

	int writebufsize()
	{
		return 128;
	}
};

template<class charT=char,
		 class rwT=CDefaultReadWrite<charT>,
		 class traits = char_traits<charT> >
class CRwStreamBuf: public basic_streambuf<charT,traits>
{
private:
	rwT& _rw;
	charT *_out;
	streamsize _out_len;
	charT *_in;
	streamsize _in_len;
	ios_base::openmode _mode;

	CRwStreamBuf(const CRwStreamBuf&);
	CRwStreamBuf& operator=(const CRwStreamBuf&);

public:
	typedef charT   char_type;
	typedef typename traits::int_type        int_type;
	typedef typename traits::pos_type        pos_type;
	typedef typename traits::off_type        off_type;
	typedef traits                  traits_type;
	CRwStreamBuf(rwT& rw, ios_base::openmode mode = ios_base::in | ios_base::out | ios_base::binary):
		_rw(rw),_out(0),_out_len(0),_in(0),_in_len(0),_mode(mode)
	{
		this->setg(_in,_in,_in);
		this->setp(_out,_out + _out_len);
	}
	virtual ~CRwStreamBuf()
	{
		if(_out)
			free_out_buf();
		if(_in)
			free_in_buf();
	}

private:
	void init_out_buf()
	{
		_out_len = _rw.writebufsize();
		_out = new charT[_out_len];
		this->setp(_out,_out + _out_len);
	}
	void free_out_buf()
	{
		delete [] _out;
		_out = 0;
		_out_len = 0;
		this->setp(_out, _out);
	}
	void init_in_buf()
	{
		_in_len = _rw.readbufsize();
		_in = new charT[_in_len];
		this->setg(_in,_in,_in);
	}
	void free_in_buf()
	{
		delete [] _in;
		_in = 0;
		_in_len = 0;
		this->setg(_in,_in,_in);
	}

protected:
	virtual int_type overflow(int_type c)
	{
		if((_mode & ios_base::out) == 0)
			return traits::eof();
		sync();
		if(traits::eq_int_type(c,traits::eof()) )
			return traits::not_eof(c);
		else
		{
			if(this->pptr() == 0)
				init_out_buf();
			return this->sputc(c);
		}
	}
	virtual int_type underflow()
	{
		if((_mode & ios_base::in) == 0)
			return traits::eof();
		if(this->egptr() == 0)
			init_in_buf();
		if(this->egptr() == _in + _in_len)
			this->setg(_in,_in,_in);
		int tmp = _rw.read(this->egptr(), _in + _in_len - this->egptr());
		if(tmp < 1)
			return traits::eof();
		this->setg(this->eback(),this->egptr(),this->egptr() + tmp);
		return traits::to_int_type(*this->gptr());
	}
	virtual int sync()
	{
		if(this->pptr() > this->pbase())
		{
			int tmp = this->pptr() - this->pbase();
			int tmp1 = _rw.write(this->pbase(), tmp);
			if(tmp1 < 1)
				return -1;
			if(tmp1 < tmp)
			{
				FOCP_NAME::CBinary::MemoryCopy(this->pbase(),this->pbase() + tmp1, tmp - tmp1);
				this->setp(this->pbase(),this->epptr());
				this->pbump(tmp1);
			}
			else
				this->setp(this->pbase(),this->epptr());
		}
		return 0;
	}
};

template<class charT=char,
		 class rwT=CDefaultReadWrite<charT>,
		 class traits = char_traits<charT> >
class CRwStream
{
private:
	CRwStreamBuf<charT, rwT, traits> m_oBuf;
	basic_iostream<charT,traits>* m_pStream;

public:
	virtual ~CRwStream()
	{
		delete m_pStream;
	}
	explicit CRwStream(rwT& rw, ios_base::openmode mode = ios_base::in | ios_base::out | ios_base::binary):
		m_oBuf(rw, mode)
	{
		m_pStream = new basic_iostream<charT,traits>(&m_oBuf);
	}
	basic_iostream<charT,traits>* GetStream()
	{
		return m_pStream;
	}
};

}

#endif
