import abc
import asyncio
import ctypes
from typing import Union

from .define import MaaStatus, MaaId


class Status:
    _status: MaaStatus

    def __init__(self, status: Union[ctypes.c_int32, MaaStatus]):
        self._status = MaaStatus(status)

    def done(self) -> bool:
        """
        Check if the status is done.

        :return: True if the status is done, False otherwise.
        """

        return self._status in [MaaStatus.success, MaaStatus.failure]

    def success(self) -> bool:
        """
        Check if the status is success.

        :return: True if the status is success, False otherwise.
        """

        return self._status == MaaStatus.success

    def failure(self) -> bool:
        """
        Check if the status is failure.

        :return: True if the status is failure, False otherwise.
        """

        return self._status == MaaStatus.failure

    def pending(self) -> bool:
        """
        Check if the status is pending.

        :return: True if the status is pending, False otherwise.
        """

        return self._status == MaaStatus.pending

    def running(self) -> bool:
        """
        Check if the status is running.

        :return: True if the status is running, False otherwise.
        """

        return self._status == MaaStatus.running


class Future(abc.ABC):
    _mid: MaaId

    def __init__(self, mid: MaaId, status_func):
        self._mid = mid
        self._status_func = status_func

    async def wait(self) -> bool:
        while not self.status().done():
            await asyncio.sleep(0)

        return self.success()

    def status(self) -> Status:
        return Status(self._status_func(self._mid))

    def done(self) -> bool:
        return self.status().done()

    def success(self) -> bool:
        return self.status().success()

    def failure(self) -> bool:
        return self.status().failure()

    def pending(self) -> bool:
        return self.status().pending()

    def running(self) -> bool:
        return self.status().running()
