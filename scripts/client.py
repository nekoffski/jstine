import jstine
import asyncio


async def main():
    async with jstine.AsyncClient(port=9991) as c:
        pong = await c.ping(payload=b"Hello world!")
        print(pong)


asyncio.run(main())
