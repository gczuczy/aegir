export class Program {
    constructor(
	public id: number,
	public name: string,
	public starttemp: number,
	public endtemp: number,
	public boiltime: number,
	public mashsteps: ProgramMashTemp[],
	public hops: ProgramHop[]
    ) {}
}

export class ProgramMashTemp {
    constructor(
	public order: number,
	public temp: number,
	public holdtime: number
    ){}
}

export class ProgramHop {
    constructor (
	public attime: number,
	public name: string,
	public quantity: number
    ){}
}
