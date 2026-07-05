import jstine
import asyncio


async def main():
    async with jstine.AsyncClient(port=9991) as c:
        resp = await c.set(key=b"foo", value=b"bar")
        print(resp)


asyncio.run(main())
