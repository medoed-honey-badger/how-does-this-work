modded class SCR_ResourceComponent : ScriptComponent
{
	
	
	//------------------------------------------------------------------------------------------------
	//! Use this function to obtain the Resource component instead of IEntity.FindComponent(SCR_ResourceComponent)
	//! \param[in] entity Entity to get Resource component from. Will loop through children in entity is a vehicle and no resource component was found on the entity
	//! \param[in] ignoreChildIfHasStorage If looping through children and this is true than it will not return the ResourceComponent if that child has an inventory storage
	//! \return Get ResourceComponent from given entity.
	override static SCR_ResourceComponent FindResourceComponent(IEntity entity, bool ignoreChildIfHasStorage = false)
	{
		// Мой код
		Print("SCR_ResourceComponent::SCR_ResourceComponent DEBUG");
		
		//~ Function is used in many places. Not all can guarantee that entity is not null
		if (!entity)
			return null;
		
		//~ Get resource component
		SCR_ResourceComponent resourceComponent = SCR_ResourceComponent.Cast(entity.FindComponent(SCR_ResourceComponent));
		if (resourceComponent)
			return resourceComponent;
		
		//~ Loop over slotted entities if entity is a vehicle. Note that slotted entities are not yet in hierarchy the moment they are created for clients
		if (Vehicle.Cast(entity))
		{
			SlotManagerComponent slotManager = SlotManagerComponent.Cast(entity.FindComponent(SlotManagerComponent));
			if (!slotManager)
				return null;
			
			array<EntitySlotInfo> outSlotInfos = {};
			slotManager.GetSlotInfos(outSlotInfos);
			IEntity slot;
			
			foreach (EntitySlotInfo slotInfo : outSlotInfos)
			{
				slot = slotInfo.GetAttachedEntity();
				
				if (!slot)
					continue;
				
				resourceComponent = SCR_ResourceComponent.Cast(slot.FindComponent(SCR_ResourceComponent));
				if (resourceComponent)
				{					
					//~ It does not care if the slotted entity has a storage or not
					if (!ignoreChildIfHasStorage)
						return resourceComponent;
					
					//~ Has no storage so this is a valid resource component
					if (!slot.FindComponent(ScriptedBaseInventoryStorageComponent))
						return resourceComponent;
				}
			}
		}
		//~ Check if parent is vehicle than get resource component from parent or any other slot
		else if (Vehicle.Cast(entity.GetParent()))
		{
			InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
			if (inventoryItem)
			{
				//~ Is item in inventory thus ignore getting vehicle parent
				if (inventoryItem.GetParentSlot())
					return null;
			}
			
			return SCR_ResourceComponent.FindResourceComponent(entity.GetParent(), ignoreChildIfHasStorage);
		}
		
		return null;
	}
}
