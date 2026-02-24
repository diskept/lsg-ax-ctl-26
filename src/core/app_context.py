"""Global application context with lazy-initialized services."""
from __future__ import annotations

from typing import TYPE_CHECKING

from src.services.node_manager import NodeManager

if TYPE_CHECKING:
    pass  # TODO: add ConfigService, LogService when implemented


class AppContext:
    """Holds global services. Services are lazily initialized on first access."""

    __slots__ = ("_node_manager",)

    def __init__(self) -> None:
        self._node_manager: NodeManager | None = None

    @property
    def node_manager(self) -> NodeManager:
        if self._node_manager is None:
            self._node_manager = NodeManager()
        return self._node_manager
