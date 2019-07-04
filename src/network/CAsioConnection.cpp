#include "network/CAsioConnection.h"
#include "network/BaseNetService.h"
#include "network/NetMessage.h"
#include "common/log.h"
#include <list>

namespace network{

	AsioConnection::AsioConnection(BaseNetService& net, bool encrypt)
		:net_(net), socket_(net.get_ioservice()), encrypt_(encrypt), is_connected_(false)
	{
		pid_ = 0;
		pz_ = NULL;
		if(encrypt_)
		{
			pz_ = new Lz4Compressor();
		}
		sendbuf_.reserve(2*NetMessage::max_body_length);//开辟2个最大包体的缓冲区
		write_in_progress_ = false;
		heart_beat_tag_ = 1;
		limit_msg_num_ = 0;
		limit_invalid_num_ = 0;
		last_check_timestamp_ = 0;
		msg_count_ = 0;
		invalid_count_ = 0;
#ifdef SERVER_DEBUG
		nAskSend = 0;
		nRealSend = 0;
		nRawRecv = 0;
		nDecomRecv = 0;
#endif
	}

	AsioConnection::~AsioConnection()
	{
		SAFE_DELETE(pz_);
	}

	BaseNetService& AsioConnection::get_net()
	{
		return net_;
	}

	tcp::socket& AsioConnection::get_socket()
	{
		return socket_;
	}

	NetMessage& AsioConnection::get_read_buffer()
	{
		return read_msg_;
	}

	Lz4Compressor& AsioConnection::get_z()
	{
		return *pz_;
	}

	bool AsioConnection::is_encrypt()
	{
		return encrypt_;
	}

	bool AsioConnection::is_connected()
	{
		return is_connected_;
	}

	void AsioConnection::on_connected()
	{
		is_connected_ = true;
	}

	bool AsioConnection::send(t_proto proto_type, const std::shared_ptr<std::string>& buf)
	{
		std::shared_ptr<std::string> zbuf = buf;
		if (encrypt_)
		{
			zbuf = std::make_shared<std::string>();
			if (pz_->compress(zbuf, buf) == 0)
			{
				LOG_ERROR("conn %d SEND DATA failed with compress return 0", get_pid());
				return false;
			}
		}
#ifdef SERVER_DEBUG
		nAskSend += (int)buf->size();
		nRealSend += (int)zbuf->size();
		LOG_INFO("conn %d SEND DATA size %d, comsize %d, total asksend %d, total realsend %d", get_pid(), buf->size(), zbuf->size(), nAskSend, nRealSend);
#endif
		NetMessage msg;
		if (!msg.body_length((uint32)zbuf->size()))
		{
			LOG_ERROR("conn %d SEND DATA failed with compressed size %d exceeds netmessage length", get_pid(), zbuf->size());
			return false;
		}
		std::memcpy(msg.body(), zbuf->c_str(), msg.body_length());
		msg.encode_header(proto_type);
		sendbuf_.append(msg.data(), msg.length());
		if (!write_in_progress_)
		{
			do_write();
		}
		
		return true;
	}

	bool AsioConnection::recv(const std::shared_ptr<std::string>& buf)
	{
		if (encrypt_)
		{
			std::shared_ptr<std::string> zbuf = std::make_shared<std::string>(get_read_buffer().body(), get_read_buffer().body_length());
			bool ret = pz_->uncompress(buf, zbuf) != 0;
#ifdef SERVER_DEBUG
			if(ret)
			{
				nRawRecv += (int)zbuf->size();
				nDecomRecv += (int)buf->size();
				LOG_INFO("conn %d recv data size %d, decomsize %d, total rawrecv %d, total realrecv %d", get_pid(), zbuf->size(), buf->size(), nRawRecv, nDecomRecv);
			}
#endif
			return ret;
		}
		else
		{
			buf->assign(get_read_buffer().body(), get_read_buffer().body_length());
#ifdef SERVER_DEBUG
			nRawRecv += (int)buf->size();
			LOG_INFO("conn %d recv data size %d, total rawrecv %d", get_pid(), buf->size(), nRawRecv);
#endif
			return true;
		}
	}

	void AsioConnection::set_pid(uint32 pid)
	{
		pid_ = pid;
	}

	uint32 AsioConnection::get_pid()
	{
		return pid_;
	}

	std::string& AsioConnection::get_peer_ip()
	{
		return ip_;
	}

	bool AsioConnection::close()
	{
		if (!is_connected_)
		{
			return false;
		}
		is_connected_ = false;
		asio::error_code err;
		asio::ip::tcp::socket& sock = get_socket();
		sock.shutdown(asio::ip::tcp::socket::shutdown_both, err);
		if (err)
		{
			// An error occurred.
			LOG_ERROR("sock shutdown with error %d, pid %d, msg %s", err.value(), get_pid(), err.message().c_str());
			return false;
		}

		sock.close(err);
		if (err)
		{
			// An error occurred.
			LOG_ERROR("sock close with error %d, pid %d, msg %s", err.value(), get_pid(), err.message().c_str());
			return false;
		}
		return true;
	}

	void AsioConnection::do_write()
	{
		write_in_progress_ = true;
		std::shared_ptr<AsioConnection> this_conn = shared_from_this();//异步线程去处理，所以要保证引用不释放
		asio::async_write(socket_, asio::buffer(sendbuf_.data(), sendbuf_.size()),
			[this_conn](const asio::error_code& ec, std::size_t length)
		{
			if (!ec)
			{
				if (!this_conn->sendbuf_.empty())
				{
					this_conn->do_write();
				}
				else
				{
					this_conn->write_in_progress_ = false;
				}
			}
			else
			{
				LOG_ERROR("do_write failed err %d, shutdown %d, errmsg %s", ec.value(), this_conn->get_pid(), ec.message().c_str());
				this_conn->get_net().do_close(*this_conn);
			}
		});
		sendbuf_.clear();
	}

	void AsioConnection::do_read_header()
	{
		std::shared_ptr<AsioConnection> this_conn = shared_from_this();//异步线程去处理，所以要保证引用不释放
		asio::async_read(get_socket(), asio::buffer(get_read_buffer().data(), NetMessage::header_length),
			[this_conn](const asio::error_code& ec, std::size_t /*length*/)
		{
			if (!ec)
			{
				if (this_conn->get_read_buffer().decode_header())
				{
					//valid packet
					this_conn->do_read_body();
				}
				else
				{
					LOG_ERROR("decode_header failed, shutdown %d", this_conn->get_pid());
					this_conn->get_net().do_close(*this_conn);
				}
			}
			else
			{
				LOG_ERROR("do_read_header failed err %d, shutdown %d, errmsg %s", ec.value(), this_conn->get_pid(), ec.message().c_str());
				this_conn->get_net().do_close(*this_conn);
			}
		});
	}

	void AsioConnection::do_read_body()
	{
		std::shared_ptr<AsioConnection> this_conn = shared_from_this();//异步线程去处理，所以要保证引用不释放
		asio::async_read(get_socket(), asio::buffer(get_read_buffer().body(), get_read_buffer().body_length()),
			[this_conn](const asio::error_code& ec, std::size_t /*length*/)
		{
			if (!ec)
			{
				std::shared_ptr<std::string> buf = std::make_shared<std::string>();
				if (!this_conn->recv(buf))
				{
					//error occured
					LOG_ERROR("do_read_body failed, shutdown %d", this_conn->get_pid());
					this_conn->get_net().do_close(*this_conn);
					return;
				}
				this_conn->get_net().on_recvmsg_cb(*this_conn, this_conn->get_read_buffer().get_prototype(), buf);
				// 消息计数器增加
				if (this_conn->is_need_msg_num_checker())
				{
					this_conn->add_msg_counter();
				}
				this_conn->do_read_header();
			}
			else
			{
				LOG_ERROR("do_read_body failed err %d, shutdown %d, errmsg %s", ec.value(), this_conn->get_pid(), ec.message().c_str());
				this_conn->get_net().do_close(*this_conn);
			}
		});
	}

	void AsioConnection::OnHeartBeat()
	{
		heart_beat_tag_ += 1;
	}

	uint8 AsioConnection::GetHeartBeatTag()
	{
		return heart_beat_tag_;
	}

	uint8 AsioConnection::SetHeartBeatTag(uint8 newTag)
	{
		return heart_beat_tag_ = newTag;
	}

	void AsioConnection::add_msg_counter()
	{
		int64 now_timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (now_timestamp == last_check_timestamp_)
		{	// 当前还是同一秒内
			msg_count_ += 1;
		}
		else
		{	// 跨了一秒, 重置次数,设置新时间戳
			msg_count_ = 1;
			last_check_timestamp_ = now_timestamp;
		}

		if (msg_count_ >= limit_msg_num_ * limit_invalid_num_)
		{
			// 一秒内, 超过了最大限制（每秒限制 * 非法次数）, 直接干掉
			LOG_INFO("connect %d msg_count %d within a second reach max times at timestamp %lld", get_pid(), msg_count_, last_check_timestamp_);
			get_net().do_close(*this);
			sendbuf_.clear();
		}
		else if (msg_count_ >= limit_msg_num_)
		{	
			// 消息数量大于单秒限制数量
			if (now_timestamp - last_invalid_timestamp_ == 1)
			{
				// 上一次非法时间是上一秒，即连续违法，增加违法次数，更新违法时间戳
				invalid_count_++;
				LOG_INFO("connect %d msg_count %d reach limit %d, invalid count %d in timestamp %lld", get_pid(), msg_count_, limit_msg_num_, invalid_count_, last_check_timestamp_);
				last_invalid_timestamp_ = now_timestamp;
			}
			else if (now_timestamp - last_invalid_timestamp_ > 1)
			{	
				// 不是连续非法，重新计数
				invalid_count_ = 1;
				last_invalid_timestamp_ = now_timestamp;
			}
			//else {
			//	// 这一秒的非法已记录，不做处理
			//}

			// 检查是否非法次数超过最大
			if (invalid_count_ >= limit_invalid_num_)
			{
				LOG_INFO("connect %d invalid count %d reach limit %d in timestamp %lld", get_pid(), invalid_count_, limit_invalid_num_, last_check_timestamp_);
				get_net().do_close(*this);
				sendbuf_.clear();
			}
		}
		//else {
		//	// 没有超过单秒和总和的限制，不做处理
		//}
	}

	bool AsioConnection::is_need_msg_num_checker()
	{
		return limit_msg_num_ > 0 && limit_invalid_num_ > 0;
	}
};