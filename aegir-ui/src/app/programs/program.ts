export class Program {
    constructor(
	public id: number,
	public name: string,
	public starttemp: number,
	public endtemp: number,
	public boiltime: number,
	public mashsteps: ProgramMashTemp[],
	public hops: ProgramHops[]
    ) {}
}

export class ProgramMashTemp {
    constructor(
	public order: number,
	public temp: number,
	public holdtime: number
    ){}
}

export class ProgramHops {
    constructor (
	public attime: number,
	public name: string,
	public quantity: number
    ){}
}
