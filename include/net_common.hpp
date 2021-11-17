#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <optional>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#ifndef NET_COMMOM_H
#define NET_COMMON_H
namespace hsc {
	namespace queues {
		template <typename T>
		class thread_safe_queue{
		public:
			thread_safe_queue() = default;
			thread_safe_queue(const thread_safe_queue<T>&) = delete;
			virtual ~thread_safe_queue() {clear();}

		public:
			const T& front(){
				std::scoped_lock lock(muxQueue);
				return deqQueue.front();
			}

			const T& back(){
				std::scoped_lock lock(muxQueue);
				return deqQueue.back();
			}

			const T& push_back(const T& item){
				std::scoped_lock lock(muxQueue);
				return deqQueue.emplace_back(std::move(item));
			}

			const T& push_front(const T& item){
				std::scoped_lock lock(muxQueue);
				return deqQueue.emplace_front(std::move(item));
			}

			bool empty(){
				std::scoped_lock lock(muxQueue);
				return deqQueue.empty();
			}

			size_t count(){
				std::scoped_lock lock(muxQueue);
				return deqQueue.size();
			}

			void clear(){
				std::scoped_lock lock(muxQueue);
				deqQueue.clear();
			}

			T pop_front(){
				std::scoped_lock lock(muxQueue);
				auto t = std::move(deqQueue.front());
				deqQueue.pop_front();
				return t;
			}

			T pop_back(){
				std::scoped_lock lock(muxQueue);
				auto t = std::move(deqQueue.back());
				deqQueue.pop_back();
				return t;
			}

		protected:
			std::mutex muxQueue;
			std::deque<T> deqQueue;
			
		};
	}
}
#endif
#include <net_message.hpp>